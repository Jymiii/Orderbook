# Orderbook

A high-performance limit order book engine implemented in C++. The project simulates the core matching engine found in financial exchanges, supporting multiple order types, price-time priority matching, and a synthetic order generator for benchmarking and simulation.

---

## Features

- **Price-Time Priority Matching** — Bids and asks are matched at the best available price, with FIFO ordering at each price level
- **Multiple Order Types** — GoodTillCancel, GoodForDay, Market, FillAndKill, and FillOrKill
- **Thread-Safe Design** — All public operations are protected by a mutex; GoodForDay orders are pruned at market close (16:30) on a background thread
- **Array-Based Level Storage** — Uses a fixed-size `LevelArray` template instead of a `std::map` for O(1) best-price lookup and cache-friendly access
- **Synthetic Order Generator** — Generates realistic order flow using geometric Brownian motion for price simulation
- **CSV Persistence** — Orders can be loaded from and saved to CSV files for replay and benchmarking
- **Built-in Profiling** — Tracks and reports average latency for add, cancel, and modify operations on shutdown

---

## Order Types

| Type | Behaviour |
|---|---|
| `GoodTillCancel` | Rests in the book until explicitly cancelled or fully filled |
| `GoodForDay` | Like GoodTillCancel, but automatically cancelled at market close (16:30) |
| `Market` | Converted to a FillAndKill at the current worst price on the opposite side |
| `FillAndKill` | Fills as much as possible immediately; any remainder is cancelled |
| `FillOrKill` | Fills completely or is cancelled entirely — no partial fills |

---

## Project Structure

```
.
├── src/
│   ├── orderbook/
│   │   ├── Orderbook.h / Orderbook.cpp   # Core matching engine
│   │   ├── Order.h                        # Order representation
│   │   ├── OrderModify.h                  # Modify request wrapper
│   │   ├── OrderType.h                    # Order type enum
│   │   ├── Side.h                         # Buy / Sell enum
│   │   ├── Trade.h                        # Filled trade record
│   │   ├── LevelArray.h                   # Fixed-size price level storage
│   │   ├── LevelData.h / LevelInfo.h      # Per-level quantity tracking
│   │   ├── OrderbookLevelInfos.h          # Snapshot of bid/ask levels
│   │   ├── Constants.h                    # LEVELARRAY_SIZE, market close time, etc.
│   │   └── Usings.h                       # Type aliases (Price, Quantity, OrderId)
│   └── synthetic_order_generator/
│       ├── OrderGenerator.h / .cpp        # Simulates order flow via GBM
│       ├── OrderExecutor.h / .cpp         # Drives the orderbook from simulation or CSV
│       ├── OrderEvent.h                   # Variant type for New / Modify / Cancel events
│       ├── OrderRegistry.h                # Tracks live orders for the generator
│       ├── MarketState.h                  # GBM parameters (mid, drift, sigma, dt)
│       └── Timer.h                        # High-resolution elapsed-time utility
├── tests/orderbook/
│   ├── GoodTillCancelTest.cpp
│   ├── GoodForDayTest.cpp
│   ├── MarketOrderTest.cpp
│   ├── FillAndKillTest.cpp
│   ├── FillOrKillTest.cpp
│   ├── OrderModifyTest.cpp
│   ├── AdditionalTests.cpp
│   └── TestHelpers.h
├── data/
│   └── orders.txt                         # Sample order data for CSV replay
├── main.cpp                               # Entry point
└── CMakeLists.txt
```

---

## Building

**Requirements:** C++23, CMake 3.20+, GCC 11+ or Clang 14+

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
./build/Orderbook
```

---

## Running

The default entry point runs an `OrderExecutor` with 10,000 simulated ticks using default `MarketState` parameters. If `data/orders.txt` exists, it replays from that CSV instead.

On exit, the engine prints average latency per operation:

```
Average time for an add: 163.412ns {Total time spend: 0.00823937, Count: 50421}
Average time for a cancel: 38.0136ns {Total time spend: 0.00170579, Count: 44873}
```

---

## Running Tests

The test suite uses [Google Test](https://github.com/google/googletest):

```bash
ctest --test-dir build --output-on-failure
```

Tests cover all five order types, order modification, level aggregation, matching edge cases, and GoodForDay pruning behaviour.

---

## Direct API Usage

```cpp
Orderbook ob;

ob.addOrder(Order{1, OrderType::GoodTillCancel, Side::Buy,  10050, 100});
ob.addOrder(Order{2, OrderType::GoodTillCancel, Side::Sell, 10045, 50});

ob.cancelOrder(1);
ob.modifyOrder(OrderModify{2, Side::Sell, 10040, 75});

OrderbookLevelInfos info = ob.getOrderInfos();
const Trades& trades = ob.getTrades();
```

---

## Design Notes

**LevelArray** is the core performance structure. Rather than `std::map<Price, Orders>`, it uses a `std::array` of size 60,000 indexed directly by price. Best and worst price pointers are maintained incrementally on every add and remove, keeping the matching loop overhead minimal with no tree traversal or heap allocation per operation.

**Market orders** are converted internally to FillAndKill orders priced at the worst available price on the opposite side, ensuring they sweep as much liquidity as possible before any remainder is discarded.

**GoodForDay pruning** runs on a dedicated background thread that sleeps until 16:30 each day, then cancels all GoodForDay orders atomically under the orderbook mutex.

---

## Synthetic Order Generator

The `OrderGenerator` models mid-price evolution using geometric Brownian motion:

```
mid(t+dt) = mid(t) * exp((drift - 0.5 * sigma^2) * dt + sigma * sqrt(dt) * Z)
```

where `Z ~ N(0,1)`. Each tick produces a Poisson-distributed burst of events split roughly 55% adds, 45% cancels.

**Default MarketState parameters:**

| Parameter | Value | Description |
|---|---|---|
| `mid` | 100.0 | Starting mid price |
| `drift` | 0.1 | Annual drift |
| `sigma` | 0.2 | Annual volatility |
| `dt` | 0.0001 | Time step per tick |
| `b` | 0.001 | Spread distribution decay |

---

## Dependencies

- C++23 standard library only (no external runtime dependencies)
- POSIX `localtime_r` for GFD pruning (Linux / macOS)
- [Google Test](https://github.com/google/googletest) for the test suite
