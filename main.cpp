#include "orderbook/Orderbook.h"
#include "simulation/direct_execution/StreamCommandExecutor.h"
#include "simulation/generation/MarketState.h"
#include "simulation/generation/RandomEngine.h"
#include "simulation/generation/StreamCommandGenerator.h"
#include "spsc_queue.h"

#include <atomic>
#include <iostream>
#include <thread>

int main()
{
    spsc_queue<Command> commandQueue{1 << 17};
    constexpr MarketState state{};
    RandomEngine rng{state.eventsPerTick};
    std::atomic<bool> stopFlag{false};
    Orderbook orderbook{};

    StreamCommandGenerator generator{commandQueue, state, rng, stopFlag};
    StreamCommandExecutor executor{commandQueue, orderbook, stopFlag};

    std::thread generatorThread{[&generator] { generator.run(); }};
    std::thread executorThread{[&executor] { executor.run(); }};

    char c;
    while (std::cin.get(c) && c != 'q') {
        std::cout << "Press 'q' to quit...\n";
    }

    stopFlag.store(true, std::memory_order_release);
    executorThread.join();
    generatorThread.join();
}
