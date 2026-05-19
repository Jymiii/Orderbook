#include "orderbook/Orderbook.h"
#include "simulation/direct_execution/StreamCommandExecutor.h"
#include "simulation/generation/MarketState.h"
#include "simulation/generation/RandomEngine.h"
#include "simulation/generation/StreamCommandGenerator.h"
#include "spsc_queue.h"

#include <atomic>
#include <thread>

int main()
{
    spsc_queue<Command> commandQueue{1 << 17};
    constexpr MarketState state{};
    RandomEngine rng{};
    std::atomic<bool> stopFlag{false};
    Orderbook orderbook{};

    StreamCommandGenerator generator{commandQueue, state, rng, stopFlag};
    StreamCommandExecutor executor{commandQueue, orderbook, stopFlag};

    std::thread generatorThread{[&generator] { generator.run(); }};
    std::thread executorThread{[&executor] { executor.run(); }};

    executorThread.join();
    generatorThread.join();
}
