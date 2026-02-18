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

        const int addCount = static_cast<int>(std::abs(getRandom() * 4));
        const int cancelCount = static_cast<int>(std::abs(getRandom() * 4));
        const int modifyCount = static_cast<int>(std::abs(getRandom()));

        std::vector<OrderEvent> eventBucket;
        eventBucket.reserve(addCount + cancelCount + modifyCount);

        generateAddOrderEvents(mid, addCount, eventBucket);
        generateCancelOrderEvents(cancelCount, eventBucket);
        generateModifyOrderEvents(mid, modifyCount, eventBucket);

        std::shuffle(eventBucket.begin(), eventBucket.end(), rng);
        for (const auto &event: eventBucket) {
            std::visit(Overloaded{
                    [&](Order const &o) {
                        registry.onNew(o);
                    },
                    [&](OrderModify const &m) {
                        registry.onModify(m);
                    },
                    [&](OrderId const &id) {
                        registry.onCancel(id);
                    }
            }, event.payload);
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

        out.emplace_back(
                EventType::New,
                Order{id++, OrderType::GoodTillCancel, side, px, getRandomQuantity()}
        );
    }
}

void OrderGenerator::generateCancelOrderEvents(int cancelCount, std::vector<OrderEvent> &out) {
    if (cancelCount <= 0) return;

    out.reserve(out.size() + static_cast<size_t>(cancelCount));

    for (int i = 0; i < cancelCount; ++i) {
        auto order = registry.randomLive(rng);
        if (!order.has_value()) return;
        out.emplace_back(EventType::Cancel, order.value().getId());
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

        out.emplace_back(
                EventType::Modify,
                OrderModify{order->getId(), side, price, quantity}
        );
    }
}
