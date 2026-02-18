
[README(1).md](https://github.com/user-attachments/files/25399302/README.1.md)
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
├── main.cpp                               # Entry point — runs OrderExecutor simulation
└── CMakeLists.txt
```

---

## Building

**Requirements:** C++23, CMake ≥ 3.20, a compiler supporting `std::to_underlying` (GCC 11+ / Clang 14+)

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

---

## Running

```bash
./build/Orderbook
```

This runs the `OrderExecutor` with a default `MarketState` for 10,000 ticks, replaying orders from `data/orders.txt` if present, otherwise generating them via simulation.

On exit, the engine prints average latency for each operation type:

```
Average time for an add: 161.792ns {Total time spend: 0.00815769, Count: 50421}
Average time for a cancel: 37.1277ns {Total time spend: 0.00166603, Count: 44873}
Average time for a modification: 1.64129ns {Total time spend: 8.085e-06, Count: 4926}
Modification info: 0.649614% went through {Total time spend: 8.085e-06, Count: 32}
```

---

## Running Tests

The test suite uses [Google Test](https://github.com/google/googletest). After building:

```bash
ctest --test-dir build --output-on-failure
```

Tests cover all five order types as well as order modification, level aggregation, matching edge cases, and GoodForDay pruning behaviour.

---

## Design Notes

**LevelArray** is the key performance structure. Rather than a `std::map<Price, Orders>`, it uses a `std::array` of size `LEVELARRAY_SIZE` (60,000) indexed directly by price. This avoids tree traversal and allocation for every price level. Best/worst price pointers are maintained incrementally on add and remove, keeping matching loop overhead minimal.

**Market orders** are converted to FillAndKill orders priced at the worst available price on the opposite side, ensuring they sweep as much liquidity as possible before any remainder is discarded.

**GoodForDay pruning** runs on a dedicated background thread that sleeps until 16:30 each day, then cancels all GoodForDay orders atomically.

---

## Synthetic Order Generator

The `OrderGenerator` models a mid-price process using geometric Brownian motion:

```
mid_{t+1} = mid_t * exp((drift - 0.5 * sigma²) * dt + sigma * sqrt(dt) * Z)
```

where `Z ~ N(0,1)`. Each tick produces a Poisson-distributed number of events split roughly 50% adds, 45% cancels, 5% modifies. Order prices are drawn from a normal distribution centred on the current mid, biased toward the inside on each side.
