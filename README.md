<div align="center">

# 📈 Orderbook

A high-performance limit order book engine in modern C++23
<em>Price-time priority matching · Array-backed price levels · Lock-free synthetic market simulation</em>

<img src="https://img.shields.io/badge/C%2B%2B-23-blue?logo=cplusplus&logoColor=white" alt="C++23"/>
<img src="https://img.shields.io/badge/build-CMake_3.25+-064F8C?logo=cmake&logoColor=white" alt="CMake 3.25+"/>
<img src="https://img.shields.io/badge/tests-Google_Test-4285F4?logo=google&logoColor=white" alt="Google Test"/>
<img src="https://img.shields.io/badge/platform-Windows%20%7C%20macOS%20%7C%20Linux-lightgrey" alt="Platform"/>

</div>

---

## Overview

This project is a from-scratch matching engine paired with a **two-thread synthetic market simulator**. A
generator thread produces a stream of order commands from a stochastic price model, hands them to the matching
engine over a **lock-free single-producer / single-consumer (SPSC) queue**, and an executor thread applies them
to the book. The engine itself owns no locks — concurrency is handled entirely by the queue.

---

## Features

| Feature | Description |
|---|---|
| **Price-Time Priority** | Bids and asks matched at the best available price; FIFO within each level |
| **Five Order Types** | GoodTillCancel, GoodForDay, Market, FillAndKill, FillOrKill |
| **Lock-Free Pipeline** | Generator and executor run on separate threads, connected by a bounded SPSC ring buffer — no mutexes in the hot path |
| **O(1) Best-Price Lookup** | Fixed-size `LevelArray<N, Side>` indexed directly by integer price tick — no tree traversal |
| **Flat-Map Order Index** | `boost::unordered_flat_map<OrderId, …>` for cache-friendly cancel / modify lookups |
| **Synthetic Market Sim** | Geometric Brownian Motion mid-price with Poisson-distributed event bursts (add / cancel / modify) |
| **Command-Driven GFD Pruning** | Good-For-Day cleanup issued as a `PruneGFDCmd` through the same command stream |
| **Built-in Profiling** | Compile-time flag prints per-operation nanosecond latencies on shutdown |

---

## Architecture

```
   ┌──────────────────────────┐        SPSC ring         ┌──────────────────────────┐
   │  StreamCommandGenerator  │   (lock-free queue)      │  StreamCommandExecutor   │
   │  ── generator thread ──  │ ───────────────────────► │  ── executor thread ──   │
   │                          │   spsc_queue<Command>    │                          │
   │  GBM mid-price model     │                          │  std::visit(cmd):        │
   │  Poisson event bursts    │                          │   NewOrderCmd  ─► add    │
   │  add / cancel / modify   │                          │   CancelOrderCmd ► cancel│
   │  OrderRegistry (live ids)│                          │   ModifyOrderCmd ► modify│
   └──────────────────────────┘                          │   PruneGFDCmd  ─► prune  │
                                                         └────────────┬─────────────┘
                                                                      │
                                                                      ▼
   ┌────────────────────────────────────────────────────────────────────────────────┐
   │                             Orderbook                                          │
   │  ┌─────────────────────┐                 ┌─────────────────────────┐           │
   │  │  LevelArray<N,Buy>  │                 │  LevelArray<N,Sell>     │           │
   │  │  (bids_)            │                 │  (asks_)                │           │
   │  │  [bestIdx_] ◄─ O(1) │                 │  [bestIdx_] ◄─ O(1)     │           │
   │  │  [worstIdx_]        │                 │  [worstIdx_]            │           │
   │  └─────────────────────┘                 └─────────────────────────┘           │
   │                                                                                │
   │  orders_: boost::unordered_flat_map<OrderId, list<Order>::iterator>            │
   │  trades_: vector<Trade>                                                        │
   └────────────────────────────────────────────────────────────────────────────────┘
```

The `Command` type is a `std::variant<NewOrderCmd, CancelOrderCmd, ModifyOrderCmd, PruneGFDCmd>`, dispatched on
the executor side with an `Overloaded` visitor.

---

## Order Types

| Type | Behaviour |
|---|---|
| `GoodTillCancel` | Rests in the book until explicitly cancelled or fully filled |
| `GoodForDay` | Like GTC but cleared when a `PruneGFDCmd` runs at market close (16:30) |
| `Market` | Converted to FillAndKill at the worst price on the opposite side |
| `FillAndKill` | Fills as much as possible immediately; remainder is cancelled |
| `FillOrKill` | Must be filled entirely in one pass — otherwise rejected in full |

---

## How Matching Works

The engine follows a **price-time priority** algorithm whenever a new order is added or modified:

1. The best bid (highest price) and best ask (lowest price) are compared.
2. If `P_best_bid ≥ P_best_ask`, a trade occurs.
3. The traded quantity is `q_trade = min(q_bid, q_ask)`.
4. Fully filled orders are removed; partial fills remain at the front of their level.
5. Matching continues until prices no longer cross or one side is empty.
6. After matching, any stale FillAndKill order left at the top of book is pruned.

### Market Order Conversion

A Market order is internally converted to a FillAndKill at the worst available opposing price:

```text
If Buy  → use worst ask
If Sell → use worst bid
```

### FillOrKill Eligibility

Before insertion, a FillOrKill order walks the opposing book from best to worst, accumulating liquidity, and is
accepted only if `sum(Q_i from best to limit) ≥ order_quantity`.

---

## LevelArray

Instead of `std::map<Price, Orders>` (O(log n)), the engine uses a fixed-size array indexed directly by integer
price tick, with cached best/worst indices:

```cpp
template<int N, Side S>
class LevelArray {
    struct LevelSlot { Orders orders; LevelData data; };
    std::unique_ptr<LevelSlot[]> levels_ = std::make_unique<LevelSlot[]>(N);
    int bestIdx_, worstIdx_;
    bool empty_;
};
```

A `BestScanPolicy<Side>` specialisation flips the scan direction and "better-than" comparison so the same code
serves both sides (bids scan high→low, asks low→high).

| Operation | Complexity |
|---|---|
| Get orders at price | O(1) |
| Get best / worst price | O(1) |
| Add order | O(1) amortised |
| Remove order | O(k) (scan to next non-empty level) |
| Full-fill check | O(L) over occupied levels |

### Price range

Prices are stored as integer ticks (`price × TICK_MULTIPLIER`) and used directly as array indices.

With `LEVELARRAY_SIZE = 2^17 = 131,072` and `TICK_MULTIPLIER = 100`, the book covers a price range of roughly
**$0.01 – $1310.71**.

---

## Synthetic Order Generator

### Geometric Brownian Motion

The mid-price evolves each tick as:

$$
S_{t+\Delta t} = S_t \cdot \exp\left[(\mu - \tfrac{1}{2}\sigma^2)\Delta t + \sigma \sqrt{\Delta t}\, Z\right]
$$

| Parameter | Field | Default |
|---|---|---|
| mid | `mid` | 100.0 |
| volatility (σ) | `sigma` | 0.2 |
| drift (μ) | `drift` | 0.01 |
| time step (Δt) | `dt` | `1 / (252·6.5·3600·1000)` (≈ per-ms of a trading year) |
| spread scale | `b` | 0.001 |
| Z | — | N(0,1) |

Order prices are drawn around the mid with an exponentially-distributed spread, then clamped into the valid tick
range.

### Event Generation

Each tick draws an event count and splits it into add / cancel / modify buckets:

```text
n_events ~ Poisson(λ = 10)
```

The split is governed by the `addCancelModOdds` weights in `MarketState` (a `std::discrete_distribution`). Cancels
and modifies pick a live order at random from the `OrderRegistry`; the whole bucket is shuffled before being pushed
to the queue so order types interleave realistically.

---

## Project Structure

```
.
├── CMakeLists.txt
├── main.cpp                         # wires up queue, generator, executor, threads
├── data/                            # captured / replay order streams (CSV)
├── src/
│   ├── commands/                    # Command variant + Overloaded visitor
│   ├── orderbook/                   # Orderbook, LevelArray, Order, Trade, types
│   ├── simulation/
│   │   ├── generation/              # StreamCommandGenerator, EventSampler, MarketState, RandomEngine
│   │   └── direct_execution/        # StreamCommandExecutor, OrderExecutor
│   └── utils/                       # Timer, TimeUtil
└── tests/                           # Google Test suites per order type
```

---

## Building

Requires a C++23 compiler and CMake 3.25+. All third-party dependencies are fetched automatically via
`FetchContent` — no manual installation needed.

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
./build/Orderbook        # press 'q' + Enter to stop the simulation
```

> The Release build enables `-march=native` and IPO/LTO where the toolchain supports it.

Enable per-operation latency instrumentation:

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release -DORDERBOOK_ENABLE_INSTRUMENTATION=ON
cmake --build build -j
```

---

## Running Tests

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
cd build && ctest --output-on-failure
```

Test suites cover each order type and order modification:
`GoodTillCancel`, `GoodForDay`, `Market`, `FillAndKill`, `FillOrKill`, `OrderModify`, plus additional edge cases.

---

## Sample Performance Output

With `-DORDERBOOK_ENABLE_INSTRUMENTATION=ON`, the engine prints aggregate latencies on shutdown:

```
Average time for an add: 118.732ns {Total time spent: ..., Count: ...}
Average time for a cancel: 36.7075ns {Total time spent: ..., Count: ...}
```

> Numbers are illustrative and depend heavily on CPU, build flags, and the generated order mix.

---

## Configuration Constants

Defined in `src/orderbook/Constants.h`:

| Constant | Value |
|---|---|
| `LEVELARRAY_SIZE` | `1 << 17` (131,072) |
| `TICK_MULTIPLIER` | 100 |
| `INITIAL_ORDER_CAPACITY` | 200,000 |
| `INVALID_PRICE` | `std::numeric_limits<Price>::min()` |
| `MarketCloseTime` | 16:30:00 |

---

## Dependencies

All resolved at configure time via CMake `FetchContent`:

- **C++23 standard library**
- **[Boost.Unordered](https://www.boost.org/)** 1.86 — `unordered_flat_map` for the order index
- **[spsc_queue](https://github.com/Jymiii/scpc_queue)** — lock-free single-producer/single-consumer ring buffer
- **[GoogleTest](https://github.com/google/googletest)** — build-time only

Cross-platform: `TimeUtil` wraps `localtime_s` on Windows and `localtime_r` elsewhere, so the project builds on
**Windows, macOS, and Linux**.

---

## License

Provided for educational and portfolio purposes.
