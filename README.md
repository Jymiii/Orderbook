<div align="center">

# 📈 Orderbook

A high-performance limit order book engine in modern C++23  
<em>Price-time priority matching · Array-backed levels · Synthetic market simulation</em>

<img src="https://img.shields.io/badge/C%2B%2B-23-blue?logo=cplusplus&logoColor=white" alt="C++23"/>
<img src="https://img.shields.io/badge/build-CMake_3.25+-064F8C?logo=cmake&logoColor=white" alt="CMake 3.25+"/>
<img src="https://img.shields.io/badge/tests-Google_Test-4285F4?logo=google&logoColor=white" alt="Google Test"/>
<img src="https://img.shields.io/badge/platform-macOS%20%7C%20Linux-lightgrey" alt="Platform"/>

</div>

---

## Features

| Feature | Description |
|---|---|
| **Price-Time Priority** | Bids and asks matched at the best available price; FIFO within each level |
| **Five Order Types** | GoodTillCancel, GoodForDay, Market, FillAndKill, FillOrKill |
| **Thread-Safe** | All public operations guarded by `std::mutex`; GFD pruning on a background thread |
| **O(1) Best-Price Lookup** | Fixed-size `LevelArray<N, Side>` indexed directly by price — no tree traversal |
| **Synthetic Market Sim** | Geometric Brownian Motion price model with Poisson-distributed event bursts |
| **CSV Replay** | Load / save order streams for deterministic benchmarking |
| **Built-in Profiling** | Compile-time flag prints per-operation nanosecond latencies on shutdown |

---

## Architecture

```
┌──────────────────────────────────────────────────────────────┐
│                        Orderbook                             │
│  ┌─────────────────────┐     ┌─────────────────────────┐     │
│  │  LevelArray<N,Buy>  │     │  LevelArray<N,Sell>     │     │
│  │  (bids_)            │     │  (asks_)                │     │
│  │                     │     │                         │     │
│  │  [bestIdx_] ◄─ O(1) │     │  [bestIdx_] ◄─ O(1)     │     │
│  │       ...           │     │       ...               │     │
│  │  [worstIdx_]        │     │  [worstIdx_]            │     │
│  └─────────────────────┘     └─────────────────────────┘     │
│                                                              │
│  orders_: unordered_map<OrderId, list<Order>::iterator>      │
│  trades_: vector<Trade>                                      │
│                                                              │
│  ┌─────────────────────────────────────────────────────┐     │
│  │  Background thread: GoodForDay pruning @ 16:30      │     │
│  └─────────────────────────────────────────────────────┘     │
└──────────────────────────────────────────────────────────────┘
                           ▲
                           │  addOrder / cancelOrder / modifyOrder
                           │
               ┌───────────┴────────────┐
               │     OrderExecutor      │
               │  (runs simulation or   │
               │   replays from CSV)    │
               └───────────┬────────────┘
                           │
               ┌───────────┴────────────┐
               │     OrderGenerator     │
               │  (GBM price model +    │
               │   Poisson event flow)  │
               └────────────────────────┘
```

---

## Order Types

| Type | Behaviour |
|---|---|
| `GoodTillCancel` | Rests in the book until explicitly cancelled or fully filled |
| `GoodForDay` | Like GTC but automatically cancelled at market close (16:30) |
| `Market` | Converted to FillAndKill at the worst price on the opposite side |
| `FillAndKill` | Fills as much as possible immediately; remainder is cancelled |
| `FillOrKill` | Must be filled entirely in one pass — otherwise cancelled in full |

---

## How Matching Works

The engine follows a **price-time priority** algorithm whenever a new order is added or modified:

1. The best bid (highest price) and best ask (lowest price) are compared.
2. If P_best_bid ≥ P_best_ask, a trade occurs.
3. The traded quantity is:

```text
q_trade = min(q_bid, q_ask)
```

4. Fully filled orders are removed; partial fills remain at the front of their level.
5. Matching continues until prices no longer cross or one side is empty.
6. After matching, any stale FillAndKill order left at the top of book is pruned.

---

### Market Order Conversion

A Market order is internally converted to a FillAndKill at the worst available opposing price:

```text
If Buy  → use worst ask
If Sell → use worst bid
```

---

### FillOrKill Eligibility

Before insertion, a FillOrKill order walks the opposing book from best to worst, accumulating available liquidity.

The order is accepted only if:

```text
sum(Q_i from best to limit) ≥ order_quantity
```

---

## LevelArray

Instead of `std::map<Price, Orders>` (O(log n)), the engine uses a fixed-size array indexed directly by price:

```cpp
template<int N, Side S>
class LevelArray {
    struct Slot { Orders orders; LevelData data; };
    std::array<Slot, N> levels_;
    int bestIdx_, worstIdx_;
    bool empty_;
};
```

| Operation | Complexity |
|---|---|
| Get orders at price | O(1) |
| Get best price | O(1) |
| Add order | O(1) amortised |
| Remove order | O(k) (scan for new best) |
| Full-fill check | O(L) |

---

### Memory Layout

Each slot contains:

- `std::list<Order>` (FIFO queue)
- `LevelData` (total quantity + order count)

With N = 60,000 and prices stored as integer ticks:

Price range covered: **$0.00 – $600.00**  
(at `TICK_MULTIPLIER = 100`)

Mathematically:

$$
\text{Index} = \mathrm{Price}_{\mathrm{ticks}}
\qquad\Longleftrightarrow\qquad
\mathrm{Price}_{\mathrm{dollars}} = \frac{\text{Index}}{\text{TICK\_MULTIPLIER}}
$$

---

## Synthetic Order Generator

### Geometric Brownian Motion

Mid-price evolves as:

$$
S_{t+\Delta t} = S_t \cdot \exp\left[(\mu - \tfrac{1}{2}\sigma^2)\Delta t + \sigma \sqrt{\Delta t} Z\right]
$$

Where:

| Parameter | Default |
|-----------|----------|
| mid | 100.0 |
| drift (μ) | 0.1 |
| volatility (σ) | 0.2 |
| dt | 0.0001 |
| Z | N(0,1) |

---

### Event Generation

Each tick:

```text
n_events ~ Poisson(λ = 10)
```

Event weights:

| Event | Probability |
|---|---|
| New order | 55% |
| Cancel | 45% |

Cancels are processed before adds to avoid cancelling newly inserted orders.

---

## Project Structure

```
.
├── CMakeLists.txt
├── main.cpp
├── data/
│   └── orders.txt
├── src/
│   ├── orderbook/
│   ├── synthetic_order_generator/
│   └── shared/
└── tests/orderbook/
```

---

## Building

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)
./build/Orderbook
```

Enable instrumentation:

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release \
      -DORDERBOOK_ENABLE_INSTRUMENTATION=ON
cmake --build build -j$(nproc)
```

---

## Running Tests

```bash
cd build
ctest --output-on-failure
```

---

## Sample Performance Output

```
Took 0.0124942

Average time for an add: 118.732ns
Average time for a cancel: 36.7075ns
```

---

## Configuration Constants

Defined in `Constants.h`:

| Constant | Value |
|---|---|
| LEVELARRAY_SIZE | 60,000 |
| TICK_MULTIPLIER | 100 |
| INITIAL_ORDER_CAPACITY | 200,000 |
| INVALID_PRICE | INT_FAST32_MIN |
| MarketCloseTime | 16:30:00 |

---

## Dependencies

- C++23 standard library
- POSIX `localtime_r`
- Google Test (build-time only)

No external runtime libraries required.

---

## License

Provided for educational and portfolio purposes.
