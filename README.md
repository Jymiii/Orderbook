<p align="center">
  <h1 align="center">📈 Orderbook</h1>
  <p align="center">
    A high-performance limit order book engine in modern C++23
    <br/>
    <em>Price-time priority matching · Array-backed levels · Synthetic market simulation</em>
  </p>
</p>

<p align="center">
  <img src="https://img.shields.io/badge/C%2B%2B-23-blue?logo=cplusplus&logoColor=white" alt="C++23"/>
  <img src="https://img.shields.io/badge/build-CMake_3.25+-064F8C?logo=cmake&logoColor=white" alt="CMake 3.25+"/>
  <img src="https://img.shields.io/badge/tests-Google_Test-4285F4?logo=google&logoColor=white" alt="Google Test"/>
  <img src="https://img.shields.io/badge/platform-macOS%20%7C%20Linux-lightgrey" alt="Platform"/>
</p>

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
│  ┌─────────────────────┐     ┌─────────────────────────┐    │
│  │  LevelArray<N,Buy>  │     │  LevelArray<N,Sell>     │    │
│  │  (bids_)            │     │  (asks_)                │    │
│  │                     │     │                         │    │
│  │  [bestIdx_] ◄─ O(1) │     │  [bestIdx_] ◄─ O(1)    │    │
│  │       ...           │     │       ...               │    │
│  │  [worstIdx_]        │     │  [worstIdx_]            │    │
│  └─────────────────────┘     └─────────────────────────┘    │
│                                                              │
│  orders_: unordered_map<OrderId, list<Order>::iterator>      │
│  trades_: vector<Trade>                                      │
│                                                              │
│  ┌─────────────────────────────────────────────────────┐    │
│  │  Background thread: GoodForDay pruning @ 16:30      │    │
│  └─────────────────────────────────────────────────────┘    │
└──────────────────────────────────────────────────────────────┘
                           ▲
                           │  addOrder / cancelOrder / modifyOrder
                           │
               ┌───────────┴────────────┐
               │     OrderExecutor       │
               │  (runs simulation or    │
               │   replays from CSV)     │
               └───────────┬────────────┘
                           │
               ┌───────────┴────────────┐
               │     OrderGenerator      │
               │  (GBM price model +     │
               │   Poisson event flow)   │
               └────────────────────────┘
```

---

## Order Types

| Type | Behaviour |
|---|---|
| `GoodTillCancel` | Rests in the book until explicitly cancelled or fully filled |
| `GoodForDay` | Like GTC but automatically cancelled at market close (**16:30**) |
| `Market` | Converted to FillAndKill at the worst price on the opposite side |
| `FillAndKill` | Fills as much as possible immediately; remainder is cancelled |
| `FillOrKill` | Must be filled entirely in one pass — otherwise cancelled in full |

---

## How Matching Works

The engine follows a **price-time priority** algorithm everytime a new order is added or an existing order is modified:

1. The best bid (highest price) and best ask (lowest price) are compared.
2. If $P_{\text{best bid}} \geq P_{\text{best ask}}$, a trade occurs.
3. The traded quantity is the minimum of the two front-of-queue orders:

$$
q_{\text{trade}} = \min\!\bigl(q_{\text{bid}},\; q_{\text{ask}}\bigr)
$$

4. Fully filled orders are removed; partial fills remain at the front of their level.
5. Matching continues until $P_{\text{best bid}} < P_{\text{best ask}}$ or one side is empty.
6. After matching, any stale `FillAndKill` order left at the top of book is pruned.

### Market Order Conversion

A **Market** order is internally converted to a `FillAndKill` at the worst available price on the opposing side, sweeping maximum liquidity:

$$
P_{\text{market}} =
\begin{cases}
P_{\text{worst ask}} & \text{if side} = \text{Buy} \\[4pt]
P_{\text{worst bid}} & \text{if side} = \text{Sell}
\end{cases}
$$

### FillOrKill Eligibility

Before insertion, a FillOrKill order walks the opposing book from best to worst, accumulating available quantity at each level. The order is accepted only if:

$$
\sum_{i=\text{best}}^{\text{limit}} Q_i \;\geq\; q_{\text{order}}
$$

where the sum runs over all levels with price better than or equal to the order's limit price.

---

## LevelArray

Instead of a `std::map<Price, Orders>` (red-black tree, $O(\log n)$ per lookup), the engine uses a **fixed-size array** indexed directly by price:

```cpp
template<int N, Side S>
class LevelArray {
    struct Slot { Orders orders; LevelData data; };
    std::array<Slot, N> levels_;   // N = 60,000 by default
    int bestIdx_, worstIdx_;
    bool empty_;
};
```

| Operation | Complexity | Notes |
|---|---|---|
| Get orders at price | $O(1)$ | Direct array index |
| Get best price | $O(1)$ | Maintained pointer |
| Add order | $O(1)$ amortised | Update best/worst pointers |
| Remove order | $O(k)$ worst case | Scan to find new best/worst ($k$ = empty levels skipped) |
| Full-fill check | $O(L)$ | $L$ = number of price levels traversed |

The `BestScanPolicy<Side>` template specialisation controls scan direction — ascending for asks, descending for bids — so the same code handles both sides without branching.

### Memory Layout

Each slot contains a `std::list<Order>` (the FIFO queue) and a `LevelData` aggregate (total quantity + order count). With $N = 60{,}000$ and prices stored as integer ticks, this covers a price range of **\$0.00 – \$600.00** (at `TICK_MULTIPLIER = 100`), suitable for most equity instruments.

$$
\text{Index} = \text{Price}_{\text{ticks}} \qquad\Longleftrightarrow\qquad \text{Price}_{\text{dollars}} = \frac{\text{Index}}{\text{TICK_MULTIPLIER}}
$$

---

## 🎲 Synthetic Order Generator

### Geometric Brownian Motion

The mid-price evolves according to a discretised GBM:

$$
S_{t+\Delta t} \;=\; S_t \cdot \exp\!\Bigl[\bigl(\mu - \tfrac{1}{2}\sigma^2\bigr)\,\Delta t \;+\; \sigma\,\sqrt{\Delta t}\;Z\Bigr]
$$

where:

| Symbol | Parameter | Default | Description |
|---|---|---|---|
| $S_t$ | `mid` | 100.0 | Current mid-price |
| $\mu$ | `drift` | 0.1 | Annualised drift |
| $\sigma$ | `sigma` | 0.2 | Annualised volatility |
| $\Delta t$ | `dt` | 0.0001 | Time step per tick |
| $Z$ | — | — | Standard normal, $Z \sim \mathcal{N}(0,\,1)$ |

### Order Price Distribution

Individual order prices are sampled around the mid using an exponential spread model:

$$
P_{\text{order}} = \left\lfloor\,\text{TICK_MULT} \;\cdot\; S_t \;\cdot\; e^{\pm\, d}\,\right\rceil
$$

where the displacement $d$ is drawn from a reflected exponential:

$$
d = -b \;\ln\!\bigl(1 - 2\,|U|\bigr), \qquad U \sim \mathrm{Uniform}\!\bigl(-\tfrac{1}{2},\;\tfrac{1}{2}\bigr)
$$

The sign of the exponent depends on the order side — negative for bids (tighter to mid), positive for asks. The parameter $b = 0.001$ controls how concentrated orders are around the mid-price.

### Event Generation

Each tick produces a Poisson-distributed number of events:

$$
n_{\text{events}} \sim \mathrm{Poisson}(\lambda = 10)
$$

Events are drawn from a discrete distribution:

| Event | Weight | ≈ Probability |
|---|---|---|
| New order | 55 | 55% |
| Cancel | 45 | 45% |

Cancels are processed **before** adds within each tick to avoid cancelling a newly placed order in the same burst. Events are then shuffled before execution.

---

## Project Structure

```
.
├── CMakeLists.txt
├── main.cpp                               # Entry point
├── data/
│   └── orders.txt                         # Sample CSV for replay
├── src/
│   ├── orderbook/
│   │   ├── Orderbook.h / .cpp             # Core matching engine
│   │   ├── Order.h                        # Order representation
│   │   ├── OrderModify.h                  # Modify request wrapper
│   │   ├── OrderType.h                    # Enum: GTC, FAK, FOK, Market, GFD
│   │   ├── Side.h                         # Enum: Buy, Sell
│   │   ├── Trade.h                        # Filled trade record
│   │   ├── LevelArray.h                   # Fixed-size price level storage
│   │   ├── LevelData.h                    # Per-level quantity + count
│   │   ├── LevelInfo.h                    # Snapshot struct for reporting
│   │   ├── OrderbookLevelInfos.h          # Bid/ask level snapshot
│   │   ├── Constants.h                    # Array size, market close time
│   │   └── Usings.h                       # Type aliases (Price, Quantity, OrderId)
│   ├── synthetic_order_generator/
│   │   ├── OrderGenerator.h / .cpp        # GBM-based order flow simulation
│   │   ├── OrderExecutor.h / .cpp         # Drives orderbook from sim or CSV
│   │   ├── OrderEvent.h                   # Variant: New | Modify | Cancel
│   │   ├── OrderRegistry.h               # Tracks live orders for the generator
│   │   └── MarketState.h                  # GBM parameters
│   └── shared/
│       └── Timer.h                        # High-resolution elapsed-time utility
└── tests/orderbook/
    ├── GoodTillCancelTest.cpp
    ├── GoodForDayTest.cpp
    ├── MarketOrderTest.cpp
    ├── FillAndKillTest.cpp
    ├── FillOrKillTest.cpp
    ├── OrderModifyTest.cpp
    ├── AdditionalTests.cpp
    └── TestHelpers.h
```

---

## Building

**Requirements:** C++23 · CMake ≥ 3.25 · Clang 16+ or GCC 13+

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)
./build/Orderbook
```

### Enable Instrumentation

To print per-operation latency statistics on shutdown, turn on the instrumentation flag:

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release \
      -DORDERBOOK_ENABLE_INSTRUMENTATION=ON
cmake --build build -j$(nproc)
```

---

## Running

The default entry point runs `OrderExecutor` with **10,000 simulated ticks** (≈ 100k events). If `data/orders.txt` exists, it replays from CSV instead.

```bash
./build/Orderbook
```

**Sample output (with instrumentation enabled):**

```
Took 0.0124942

Average time for an add: 118.732ns {Total time spent: 0.00598661, Count: 50421}
Average time for a cancel: 36.7075ns {Total time spent: 0.00164718, Count: 44873}
```

---

## Running Tests

The test suite uses [Google Test](https://github.com/google/googletest) (fetched automatically via CMake `FetchContent`):

```bash
cd build
ctest --output-on-failure
```

Tests cover:

- ✅ All five order types (GTC, GFD, Market, FAK, FOK)
- ✅ Order modification (cancel-and-replace)
- ✅ Level aggregation and snapshot correctness
- ✅ Partial and full fill edge cases
- ✅ GoodForDay background pruning

---

## API Usage

```cpp
#include "orderbook/Orderbook.h"

Orderbook ob;

// Place orders
ob.addOrder(Order{1, OrderType::GoodTillCancel, Side::Buy,  10050, 100});
ob.addOrder(Order{2, OrderType::GoodTillCancel, Side::Sell, 10045,  50});
// → Trade: 50 units @ bid 10050 / ask 10045

// Cancel an order
ob.cancelOrder(1);

// Modify (cancel-and-replace, preserves original OrderType)
ob.modifyOrder(OrderModify{2, Side::Sell, 10040, 75});

// Query the book
OrderbookLevelInfos info = ob.getOrderInfos();
const Trades& trades = ob.getTrades();
std::cout << ob;   // pretty-print bid/ask levels
```

---

## Thread Safety & GoodForDay Pruning

All public methods (`addOrder`, `cancelOrder`, `modifyOrder`, `getOrderInfos`, …) acquire a `std::scoped_lock` on `orderMutex_` before touching internal state.

A dedicated background thread sleeps until **16:30** each day using `std::condition_variable::wait_for`. When the timeout fires it:

1. Collects all order IDs with type `GoodForDay`.
2. Cancels them atomically under the mutex.

The thread exits cleanly on destruction via a `shutdown_` flag and `notify_all`.

---

## CSV Format

Orders are persisted as one event per line:

```
<EventType>,<fields...>
```

| EventType | Fields |
|---|---|
| `0` (New) | `orderId, orderType, side, price, quantity` |
| `1` (Cancel) | `orderId` |
| `2` (Modify) | `orderId, side, price, quantity` |

**Example:**

```csv
0,1,0,0,10050,100
0,2,0,1,10045,50
1,1
2,2,1,10040,75
```

---

## Complexity Summary

| Operation | Time | Space |
|---|---|---|
| `addOrder` | $O(1)$ amortised + $O(M)$ matching | — |
| `cancelOrder` | $O(1)$ average (hash lookup + list erase) | — |
| `modifyOrder` | cancel + add | — |
| `matchOrders` | $O(M)$ where $M$ = orders matched | — |
| `getOrderInfos` | $O(L)$ where $L$ = active price levels | $O(L)$ |
| `canFullyFill` | $O(L)$ | $O(1)$ |
| `LevelArray` storage | — | $O(N)$ fixed, $N = 60{,}000$ |

---

## Configuration Constants

Defined in [`Constants.h`](src/orderbook/Constants.h):

| Constant | Value | Purpose |
|---|---|---|
| `LEVELARRAY_SIZE` | 60,000 | Array slots (price range 0 – 59,999) |
| `TICK_MULTIPLIER` | 100 | Integer ticks per dollar |
| `INITIAL_ORDER_CAPACITY` | 200,000 | Pre-allocated `unordered_map` buckets |
| `INVALID_PRICE` | `INT_FAST32_MIN` | Sentinel for market orders |
| `MarketCloseTime` | 16:30:00 | When GoodForDay orders are pruned |

---

## Dependencies

| Dependency | Purpose | Scope |
|---|---|---|
| C++23 standard library | Core implementation | Runtime |
| POSIX `localtime_r` | GFD time calculation | Runtime (Linux / macOS) |
| [Google Test](https://github.com/google/googletest) | Unit tests | Build-time only (fetched via CMake) |

No external runtime libraries are required.

---

## License

This project is provided as-is for educational and portfolio purposes.
