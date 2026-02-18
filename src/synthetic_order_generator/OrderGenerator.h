//
// Created by Jimi van der Meer on 12/02/2026.
//

#ifndef ORDERBOOK_ORDERGENERATOR_H
#define ORDERBOOK_ORDERGENERATOR_H

#include "OrderRegistry.h"
#include "MarketState.h"
#include "OrderEvent.h"

#include <random>
#include <vector>

class OrderGenerator {
public:
    std::vector<OrderEvent> generate();

    OrderGenerator(MarketState state, size_t ticks) : state_{state}, ticks_{ticks} {}

private:
    OrderRegistry registry{};
    MarketState state_{};
    size_t ticks_{};

    std::random_device dev{};
    static size_t id_;
    std::mt19937 rng{dev()};
    std::normal_distribution<double> normal_distribution{0.0, 1.0};
    std::uniform_real_distribution<double> U{-0.499999999, 0.5};
    std::uniform_real_distribution<double> uniformZeroToOne{0, 1};
    std::bernoulli_distribution bernoulliDistribution{0.5};

    Price getRandomOrderPrice(double mid, Side side);
    double getRandom() { return normal_distribution(rng); }
    double getUSample() { return U(rng); }
    Side getRandomSide();

    void generateAddOrderEvents(double mid, int addCount, std::vector<OrderEvent>& out);
    void generateCancelOrderEvents(int cancelCount, std::vector<OrderEvent>& out);
    void generateModifyOrderEvents(double mid, int modifyCount, std::vector<OrderEvent>& out);

    constexpr Quantity getRandomQuantity() noexcept;
};

#endif // ORDERBOOK_ORDERGENERATOR_H

