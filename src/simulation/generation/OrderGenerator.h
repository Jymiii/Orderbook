#pragma once

#include "EventSampler.h"
#include "MarketState.h"
#include "OrderRegistry.h"
#include "RandomEngine.h"
#include "commands/commands.h"
#include <vector>

class OrderGenerator {
public:
    OrderGenerator(const MarketState& state, size_t ticks)
        : marketState_{state}, ticks_{ticks}
    {
    }

    std::vector<Command> generate();

private:
    OrderId nextId_{0};
    OrderRegistry registry_{};
    MarketState marketState_{};
    size_t ticks_{};
    RandomEngine rng_{};
    std::discrete_distribution<int> eventTypeDist_{
        marketState_.addCancelModOdds.begin(),
        marketState_.addCancelModOdds.end()
    };
};
