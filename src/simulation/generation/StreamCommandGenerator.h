#pragma once

#include <algorithm>

#include "EventSampler.h"
#include "MarketState.h"
#include "OrderRegistry.h"
#include "RandomEngine.h"
#include "commands/commands.h"
#include "spsc_queue.h"

#include <atomic>
#include <iostream>
#include <random>
#include <vector>

class StreamCommandGenerator
{
public:
    StreamCommandGenerator(spsc_queue<Command>& queue,
                           const MarketState& marketState,
                           RandomEngine& rng,
                           std::atomic<bool>& stopFlag)
        : queue_{queue}, marketState_{marketState}, rng_{rng}, stop_{stopFlag}
    {
    }

    StreamCommandGenerator(const StreamCommandGenerator&) = delete;
    StreamCommandGenerator& operator=(const StreamCommandGenerator&) = delete;

    void run()
    {
        namespace es = event_sampler;
        double mid = marketState_.mid;
        std::vector<Command> bucket;

        while (!stop_.load(std::memory_order_acquire))
        {
            mid = es::advanceMid(mid, marketState_, rng_);
            const int eventCount = rng_.poisson();
            const auto [add, cancel, modify] = es::bucketEvents(eventCount, eventTypeDist_, rng_);

            bucket.clear();
            bucket.reserve(add + cancel + modify);

            es::sampleCancels(cancel, registry_, rng_, bucket);
            es::sampleModifies(mid, modify, registry_, marketState_, rng_, bucket);
            es::sampleAdds(mid, add, nextId_, registry_, marketState_, rng_, bucket);

            std::ranges::shuffle(bucket, rng_.engine());

            for (const auto& cmd : bucket)
            {
                while (!queue_.push(cmd) && !stop_.load(std::memory_order_acquire))
                {
                    // spin: queue full
                }
            }
        }
    }

private:
    spsc_queue<Command>& queue_;
    MarketState marketState_;
    RandomEngine& rng_;
    std::atomic<bool>& stop_;
    OrderRegistry registry_{};
    OrderId nextId_{0};
    std::discrete_distribution<int> eventTypeDist_{
        marketState_.addCancelModOdds.begin(),
        marketState_.addCancelModOdds.end()
    };
};
