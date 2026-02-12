#include "OrderGenerator.h"

#include <cmath>
#include <iostream>
#include <cstdlib>

static int id{0};
std::vector<Order> OrderGenerator::generate() {
    double mid   = state_.mid;
    double sigma = state_.sigma;
    double drift = state_.drift;
    double dt    = state_.dt;

    std::vector<Order> orders{};
    orders.reserve(ticks_);

    for (size_t i = 0; i < ticks_; ++i) {
        mid = mid * std::exp((drift - 0.5 * std::pow(sigma, 2)) * dt
                             + std::sqrt(dt) * sigma * getRandom());
        generateOrders(mid, static_cast<int>((getRandom() * 2)), orders);
    }
    return orders;
}

void OrderGenerator::generateOrders(double mid, int count, std::vector<Order>& orders) {
    if (count == 0) return;

    Side side = (count < 0) ? Side::Buy : Side::Sell;
    int n = std::abs(count);

    for (int i = 0; i < n; ++i) {
        orders.emplace_back(id++, OrderType::GoodTillCancel, side,
                            mid * getMidError(), 5);
    }
}

