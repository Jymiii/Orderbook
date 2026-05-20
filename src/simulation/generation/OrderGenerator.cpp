#include "OrderGenerator.h"

#include <algorithm>
#include <vector>

std::vector<Command> OrderGenerator::generate()
{
    namespace es = event_sampler;
    double mid = marketState_.mid;

    std::vector<Command> commands;
    commands.reserve(ticks_);

    std::vector<Command> bucket;

    for (size_t i = 0; i < ticks_; ++i) {
        mid = es::advanceMid(mid, marketState_, rng_);
        const int eventCount = rng_.poisson();
        const auto c = es::bucketEvents(eventCount, eventTypeDist_, rng_);

        bucket.clear();
        bucket.reserve(c.add + c.cancel + c.modify);

        es::sampleCancels(c.cancel, registry_, rng_, bucket);
        es::sampleModifies(mid, c.modify, registry_, marketState_, rng_, bucket);
        es::sampleAdds(mid, c.add, nextId_, registry_, marketState_, rng_, bucket);

        std::ranges::shuffle(bucket, rng_.engine());
        commands.insert(commands.end(), bucket.begin(), bucket.end());
    }
    return commands;
}
