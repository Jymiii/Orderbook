[README.md](https://github.com/user-attachments/files/25364643/README.md)
# Orderbook Engine

A high-performance, event-driven limit order book implementation in C++ with simulated market data generation and order execution infrastructure.

---

## Overview

This project implements a full limit order book (LOB) capable of matching buy and sell orders across multiple order types. It includes a stochastic market price simulator based on Geometric Brownian Motion (GBM), a flexible order executor that supports both simulated and CSV-driven order flows, and a thread-safe pruning system for time-sensitive order types.

---

## Architecture

### Core Components

**`Orderbook`** — The central matching engine. Maintains separate bid and ask `LevelArray` structures, an order lookup map, and a `LevelData` index for fast Fill-or-Kill eligibility checks. All public methods are protected by a mutex for thread safety.

**`LevelArray<N, Side>`** — A templated, fixed-capacity array of price levels. Provides O(1) best-price lookup and supports iteration from best to worst price.

**`LevelData`** — A lightweight struct tracking the quantity and order count at each price level. Used to determine whether a Fill-or-Kill order can be fully satisfied before placement.

**`OrderGenerator`** — Simulates realistic order flow using a GBM price process. At each tick, the mid price evolves stochastically and orders are placed around it with spread sampled from a Laplace-like distribution.

**`OrderExecutor`** — Orchestrates order generation and submission to the orderbook. Supports simulation mode and CSV replay mode, with optional persistence of generated orders to disk.

**`MarketState`** — A plain struct holding GBM parameters: mid price, drift (µ), volatility (σ), time step (dt), and the spread decay constant (b).

---

## Order Types

| Type | Behaviour |
|---|---|
| `GoodTillCancel` | Rests in the book until matched or explicitly cancelled |
| `GoodForDay` | Like GTC, but automatically cancelled at market close (default: end of trading day) |
| `FillAndKill` | Matches as much as possible immediately; any unfilled remainder is cancelled |
| `FillOrKill` | Executes in full immediately or is rejected entirely — never rests in the book |
| `Market` | Converted internally to a `FillAndKill` at the worst available opposing price |

---

## Matching Logic

Orders are matched inside `matchOrders()`, which runs after every `addOrderInternal()` call. The engine:

1. Iterates while the best bid ≥ best ask.
2. Trades at the quantity minimum of the two front-of-queue orders.
3. Emits a `Trade` record containing both order IDs, both prices, and the traded quantity.
4. Fully filled orders are removed from both the level queue and the order map.
5. After each matching cycle, any stale `FillAndKill` orders remaining at the top of a level are pruned.

---

## Thread Safety

The orderbook uses a single `std::mutex` (`orderMutex_`) to protect all reads and writes to internal state. A dedicated background thread runs `pruneStaleGoodForDay()`, which sleeps until market close and then cancels all `GoodForDay` orders. Shutdown is signalled via a `std::condition_variable`, allowing the prune thread to exit cleanly when the `Orderbook` is destroyed.

---

## Market Simulation

The `OrderGenerator` drives price evolution with the GBM SDE:

```
S(t+dt) = S(t) * exp((µ - ½σ²)dt + σ√dt · Z)
```

where Z ~ N(0,1). At each tick, one or more orders are generated with price offset from mid using:

```
spread = exp(±b · log(1 - 2|U|))
```

where U ~ Uniform(−0.5, 0.5), approximating a Laplace distribution. Buy orders are placed below mid (negative spread), sell orders above (positive spread).

**Default simulation parameters (`MarketState`):**

| Parameter | Value | Description |
|---|---|---|
| `mid` | 100.0 | Starting mid price |
| `drift` | 0.1 | Annual drift (µ) |
| `sigma` | 0.2 | Annual volatility (σ) |
| `dt` | 0.0001 | Time step per tick |
| `b` | 0.002 | Spread distribution decay |

---

## Usage

### Running a Simulation

```cpp
// Simulate 100,000 ticks with default market parameters
OrderExecutor executor{MarketState{}, 100000};
double elapsed_ms = executor.run();

Orderbook& ob = executor.getOrderbook();
std::cout << ob;  // Prints bid/ask level summary
```

### Replaying from CSV

Orders can be replayed from a CSV file with the format:

```
<OrderId>,<OrderType>,<Side>,<Price>,<Quantity>
```

```cpp
OrderExecutor executor{};
double elapsed_ms = executor.run("orders.csv");
```

### Persisting Generated Orders

```cpp
// Simulate and write all generated orders to disk
OrderExecutor executor{MarketState{}, 100000, "output_orders.csv"};
executor.run();
```

### Direct Orderbook Manipulation

```cpp
Orderbook ob;

ob.addOrder(Order{1, OrderType::GoodTillCancel, Side::Buy,  10050, 100});
ob.addOrder(Order{2, OrderType::GoodTillCancel, Side::Sell, 10045, 50});

// Cancel and modify
ob.cancelOrder(1);
ob.modifyOrder(OrderModify{2, 10040, 75});

// Inspect state
OrderbookLevelInfos info = ob.getOrderInfos();
const Trades& trades = ob.getTrades();
```

---

## Project Structure

```
src/
├── orderbook/                        # Core matching engine
│   ├── Constants.h                   # Market constants (close time, tick multiplier, etc.)
│   ├── LevelArray.h                  # Templated price level storage
│   ├── LevelData.h                   # Per-level quantity/count index
│   ├── LevelInfo.h                   # Read-only level snapshot struct
│   ├── Order.h                       # Order state and lifecycle
│   ├── Orderbook.h / Orderbook.cpp   # Matching engine
│   ├── OrderbookLevelInfos.h         # Bid/ask snapshot returned by getOrderInfos()
│   ├── OrderModify.h                 # Modify request wrapper
│   ├── OrderType.h                   # OrderType enum
│   ├── Side.h                        # Side enum (Buy / Sell)
│   ├── Trade.h                       # Executed trade record
│   └── Usings.h                      # Type aliases (Price, Quantity, OrderId, …)
│
└── synthetic_order_generator/        # Simulation and execution layer
    ├── MarketState.h                  # GBM parameters (mid, drift, sigma, dt, b)
    ├── OrderEvent.h                   # Order event types
    ├── OrderExecutor.h / OrderExecutor.cpp   # Simulation and CSV runner
    ├── OrderGenerator.h / OrderGenerator.cpp # GBM-based order simulator
    ├── OrderRegistry.h                # Tracks generated order IDs
    └── Timer.h                        # High-resolution elapsed timer

tests/
└── orderbook/
    ├── AdditionalTests.cpp            # Extended orderbook unit tests
    └── FillAndKillTest.cpp            # FAK / FOK specific tests                           
```

---

## Dependencies

- **C++20** or later (uses `[[maybe_unused]]`, structured bindings, `std::scoped_lock`)
- Standard library only — no external dependencies
- POSIX `localtime_r` for GFD pruning (Linux / macOS)

---

## Notes

- The orderbook is instantiated with `startPruneThread = false` inside `OrderExecutor` to avoid running the GFD background thread during benchmarking and testing.
- The global `id` counter in `OrderGenerator.cpp` is not thread-safe; order generation is assumed to be single-threaded.
- `FillOrKill` orders that somehow survive matching are treated as a logic error and will throw `std::logic_error`.


Credits / Inspiration:
https://www.youtube.com/watch?v=XeLWe0Cx_Lg (Coding Jesus)
https://www.youtube.com/watch?v=sX2nF1fW7kI&t=2256s (CPP con)

