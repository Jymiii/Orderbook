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
    std::vector<Order> generate();
    OrderGenerator(MarketState state, size_t ticks) : state_{ state }, ticks_ { ticks } {}

private:
    OrderRegistry registry{};
    MarketState state_{};
    size_t ticks_{};

    std::random_device dev{};
    static size_t id_;
    std::mt19937 rng{dev()};
    std::normal_distribution<double> normal_distribution{0.0, 1.0};
    std::uniform_real_distribution<double> real_distribution{0.95, 1};
    double getRandom() {return normal_distribution(rng);}
    double getMidError() {return real_distribution(rng);}

    void generateOrders(double mid, int count, std::vector<Order> &orders);
};

#endif // ORDERBOOK_ORDERGENERATOR_H

