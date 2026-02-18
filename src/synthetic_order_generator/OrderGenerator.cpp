#include "OrderGenerator.h"

#include <cmath>
#include <cstdlib>
#include <algorithm>
#include <vector>

static OrderId id{0};

constexpr Quantity OrderGenerator::getRandomQuantity() noexcept {
    return static_cast<Quantity>(std::abs(getRandom()) * 100 + 1);
}

int32_t OrderGenerator::getRandomOrderPrice(double mid, Side side) {
    const double uSample = getUSample();
    const double d = -state_.b * std::log(1 - 2 * std::abs(uSample));

    const double spread = (side == Side::Buy) ? std::exp(-d) : std::exp(d);
    const double raw = Constants::TICK_MULTIPLIER * mid * spread;

    return std::max<Price>(1, static_cast<Price>(std::llround(raw)));
}

constexpr OrderType OrderGenerator::getRandomOrderType() {
    int type = static_cast<int>(OrderType::Size) * uniformZeroToOne( rng );
    return static_cast<OrderType>(type);
}

Side OrderGenerator::getRandomSide() {
    return bernoulliDistribution(rng) ? Side::Sell : Side::Buy;
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
        int eventCount = eventCountRng(rng);

        int addCount = 0, cancelCount = 0, modifyCount = 0;
        for (int k = 0; k < eventCount; ++k) {
            auto type = static_cast<EventType>(eventTypeDist(rng));
            switch (type) {
                case EventType::New: ++addCount; break;
                case EventType::Cancel: ++cancelCount; break;
                case EventType::Modify: ++modifyCount; break;
            }
        }

        std::vector<OrderEvent> eventBucket;
        eventBucket.reserve(addCount + cancelCount + modifyCount);

        //Cancels/Modify first so we do not cancel orders in the same burst as we add them.
        generateModifyOrderEvents(mid, modifyCount, eventBucket);
        generateCancelOrderEvents(cancelCount, eventBucket);
        generateAddOrderEvents(mid, addCount, eventBucket);

        std::shuffle(eventBucket.begin(), eventBucket.end(), rng);
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

        auto newOrder {Order{id++, type, side, px, getRandomQuantity()}};
        out.emplace_back(
                EventType::New,
                newOrder
        );
        registry.onNew(newOrder);
    }
}

void OrderGenerator::generateCancelOrderEvents(int cancelCount, std::vector<OrderEvent> &out) {
    if (cancelCount <= 0) return;

    out.reserve(out.size() + static_cast<size_t>(cancelCount));

    for (int i = 0; i < cancelCount; ++i) {
        auto order = registry.randomLive(rng);
        if (!order.has_value()) return;
        out.emplace_back(EventType::Cancel, order.value().getId());
        registry.onCancel(order.value().getId());
    }
}

void OrderGenerator::generateModifyOrderEvents(double mid, int modifyCount, std::vector<OrderEvent> &out) {
    if (modifyCount <= 0) return;

    out.reserve(out.size() + static_cast<size_t>(modifyCount));

    for (int i = 0; i < modifyCount; ++i) {
        auto order = registry.randomLive(rng);
        if (!order.has_value()) return;

        Price price = order->getPrice();
        Quantity quantity = order->getRemainingQuantity();
        Side side = order->getSide();

        if (bernoulliDistribution(rng)) {
            quantity = getRandomQuantity();
        }
        if (bernoulliDistribution(rng)) {
            side = getRandomSide();
        }
        if (bernoulliDistribution(rng)) {
            price = getRandomOrderPrice(mid, side);
        }

        auto modify {OrderModify{order->getId(), side, price, quantity}};
        out.emplace_back(
                EventType::Modify,
                modify
        );
        registry.onModify(modify);
    }
}
