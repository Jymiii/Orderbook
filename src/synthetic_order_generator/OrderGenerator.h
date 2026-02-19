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
    OrderId nextId_{0};
    OrderRegistry registry_{};
    MarketState state_{};
    size_t ticks_{};

    std::random_device dev_{};
    std::mt19937 rng_{dev_()};
    std::normal_distribution<double> normalDist_{0.0, 1.0};
    std::uniform_real_distribution<double> uniformSpread_{-0.499999999, 0.5};
    std::uniform_real_distribution<double> uniformZeroToOne_{0, 1};
    std::bernoulli_distribution bernoulliDist_{0.5};


    static constexpr int eventsPerTick = 10;
    static constexpr std::array<double, 3> addCancelModOdds{50.0, 45.0, 5.0};

    std::poisson_distribution<int> eventCountDist_{eventsPerTick};
    std::discrete_distribution<int> eventTypeDist_{
            addCancelModOdds.begin(),
            addCancelModOdds.end()
    };

    double getRandom() { return normalDist_(rng_); }

    double getUSample() { return uniformSpread_(rng_); }

    Side getRandomSide();

    void generateAddOrderEvents(double mid, int addCount, std::vector<OrderEvent> &out);

    void generateCancelOrderEvents(int cancelCount, std::vector<OrderEvent> &out);

    [[maybe_unused]] void generateModifyOrderEvents(double mid, int modifyCount, std::vector<OrderEvent> &out);

    Price getRandomOrderPrice(double mid, Side side);

    Quantity getRandomQuantity();

    OrderType getRandomOrderType();
};

#endif // ORDERBOOK_ORDERGENERATOR_H

