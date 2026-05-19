#pragma once

#include "MarketState.h"
#include "OrderRegistry.h"
#include "RandomEngine.h"
#include "commands/commands.h"
#include "orderbook/Constants.h"
#include "orderbook/Order.h"
#include "orderbook/OrderModify.h"

#include <algorithm>
#include <cmath>
#include <random>
#include <vector>

namespace event_sampler {

struct BucketCounts { int add; int cancel; int modify; };

inline double advanceMid(double mid, const MarketState& s, RandomEngine& rng) {
    return mid * std::exp((s.drift - 0.5 * s.sigma * s.sigma) * s.dt
                          + std::sqrt(s.dt) * s.sigma * rng.normal());
}

inline BucketCounts bucketEvents(int eventCount,
                                  std::discrete_distribution<int>& dist,
                                  RandomEngine& rng)
{
    BucketCounts c{};
    for (int k = 0; k < eventCount; ++k) {
        switch (dist(rng.engine())) {
            case 0:  ++c.add;    break;
            case 1:  ++c.cancel; break;
            default: ++c.modify; break;
        }
    }
    return c;
}

namespace detail {
    inline Side randomSide(RandomEngine& rng) {
        return rng.coinflip() ? Side::Sell : Side::Buy;
    }
    inline Price randomPrice(double mid, Side side, const MarketState& s, RandomEngine& rng) {
        const double u = rng.uniformSpread();
        const double d = -s.b * std::log(1.0 - 2.0 * std::abs(u));
        const double spread = (side == Side::Buy) ? std::exp(-d) : std::exp(d);
        const double raw = Constants::TICK_MULTIPLIER * mid * spread;
        return std::clamp(static_cast<Price>(std::llround(raw)),
                          Price{1},
                          static_cast<Price>(Constants::LEVELARRAY_SIZE - 1));
    }
    inline Quantity randomQty(RandomEngine& rng) {
        return static_cast<Quantity>(std::abs(rng.normal()) * 100 + 1);
    }
    inline OrderType randomOrderType(RandomEngine& rng) {
        return static_cast<OrderType>(static_cast<int>(OrderType::Size) * rng.uniform01());
    }
}

inline void sampleAdds(double mid, int count, OrderId& nextId,
                        OrderRegistry& registry, const MarketState& s,
                        RandomEngine& rng, std::vector<Command>& out)
{
    if (count <= 0) return;
    out.reserve(out.size() + static_cast<size_t>(count));
    for (int i = 0; i < count; ++i) {
        const Side side     = detail::randomSide(rng);
        const Price px      = detail::randomPrice(mid, side, s, rng);
        const OrderType type = detail::randomOrderType(rng);
        const Quantity qty  = detail::randomQty(rng);
        const OrderId id    = nextId++;
        out.push_back(NewOrderCmd{type, side, px, id, qty});
        registry.onNew(Order{id, type, side, px, qty});
    }
}

inline void sampleCancels(int count, OrderRegistry& registry,
                           RandomEngine& rng, std::vector<Command>& out)
{
    if (count <= 0) return;
    out.reserve(out.size() + static_cast<size_t>(count));
    for (int i = 0; i < count; ++i) {
        auto order = registry.randomLive(rng.engine());
        if (!order.has_value()) return;
        out.push_back(CancelOrderCmd{order->getId()});
        registry.onCancel(order->getId());
    }
}

inline void sampleModifies(double mid, int count, OrderRegistry& registry,
                            const MarketState& s, RandomEngine& rng, std::vector<Command>& out)
{
    if (count <= 0) return;
    out.reserve(out.size() + static_cast<size_t>(count));
    for (int i = 0; i < count; ++i) {
        auto order = registry.randomLive(rng.engine());
        if (!order.has_value()) return;

        Price    price    = order->getPrice();
        Quantity quantity = order->getRemainingQuantity();
        Side     side     = order->getSide();

        if (rng.coinflip()) quantity = detail::randomQty(rng);
        if (rng.coinflip()) side     = detail::randomSide(rng);
        if (rng.coinflip()) price    = detail::randomPrice(mid, side, s, rng);

        out.push_back(ModifyOrderCmd{side, price, order->getId(), quantity});
        registry.onModify(OrderModify{order->getId(), side, price, quantity});
    }
}

} // namespace event_sampler
