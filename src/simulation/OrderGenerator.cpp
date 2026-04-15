#include "OrderGenerator.h"

#include <cmath>
#include <cstdlib>
#include <algorithm>
#include <vector>

Quantity OrderGenerator::getRandomQuantity() {
    return static_cast<Quantity>(std::abs(getRandom()) * 100 + 1);
}

Price OrderGenerator::getRandomOrderPrice(double mid, Side side) {
    const double uSample = getUSample();
    const double d = -state_.b * std::log(1 - 2 * std::abs(uSample));

    const double spread = (side == Side::Buy) ? std::exp(-d) : std::exp(d);
    const double raw = Constants::TICK_MULTIPLIER * mid * spread;

    return std::max<Price>(1, static_cast<Price>(std::llround(raw)));
}

OrderType OrderGenerator::getRandomOrderType() {
    int type = static_cast<int>(OrderType::Size) * uniformZeroToOne_(rng_);
    return static_cast<OrderType>(type);
}

Side OrderGenerator::getRandomSide() {
    return bernoulliDist_(rng_) ? Side::Sell : Side::Buy;
}

std::vector<OrderEvent> OrderGenerator::generate() {
    double mid = state_.mid;
    const double sigma = state_.sigma;
    const double drift = state_.drift;
    const double dt = state_.dt;

    std::vector<OrderEvent> orders;
    orders.reserve(ticks_);

    for (size_t i = 0; i < ticks_; ++i) {
        mid = mid * std::exp((drift - 0.5 * std::pow(sigma, 2)) * dt
                             + std::sqrt(dt) * sigma * getRandom());
        int eventCount = eventCountDist_(rng_);

        int addCount = 0, cancelCount = 0;
        for (int k = 0; k < eventCount; ++k) {
            auto type = static_cast<EventType>(eventTypeDist_(rng_));
            switch (type) {
                case EventType::New: ++addCount; break;
                case EventType::Cancel: ++cancelCount; break;
                case EventType::Modify: break;
            }
        }

        std::vector<OrderEvent> eventBucket;
        eventBucket.reserve(addCount + cancelCount);

        // Cancels first so we do not cancel orders in the same burst as we add them.
        generateCancelOrderEvents(cancelCount, eventBucket);
        generateAddOrderEvents(mid, addCount, eventBucket);

        std::shuffle(eventBucket.begin(), eventBucket.end(), rng_);
        for (const auto &event: eventBucket) {
            orders.push_back(event);
        }
    }
    return orders;
}

void OrderGenerator::generateAddOrderEvents(double mid, int addCount, std::vector<OrderEvent> &out) {
    if (addCount <= 0) return;

    out.reserve(out.size() + static_cast<size_t>(addCount));

    for (int i = 0; i < addCount; ++i) {
        const Side side = getRandomSide();
        const Price px = getRandomOrderPrice(mid, side);
        const OrderType type = getRandomOrderType();

        auto newOrder{Order{nextId_++, type, side, px, getRandomQuantity()}};
        out.emplace_back(EventType::New, newOrder);
        registry_.onNew(newOrder);
    }
}

void OrderGenerator::generateCancelOrderEvents(int cancelCount, std::vector<OrderEvent> &out) {
    if (cancelCount <= 0) return;

    out.reserve(out.size() + static_cast<size_t>(cancelCount));

    for (int i = 0; i < cancelCount; ++i) {
        auto order = registry_.randomLive(rng_);
        if (!order.has_value()) return;
        out.emplace_back(EventType::Cancel, order.value().getId());
        registry_.onCancel(order.value().getId());
    }
}
