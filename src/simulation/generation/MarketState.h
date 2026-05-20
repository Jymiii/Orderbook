#pragma once
#include <array>

struct MarketState {
    double mid = 100.0;
    double sigma = 0.2;
    double drift = 0.01;
    double dt = 1.0 / (252.0 * 6.5 * 3600.0 * 1000.0);
    double b = 0.001;
    int eventsPerTick = 10;
    std::array<double, 2> addCancelModOdds{55.0, 45.0};
};
