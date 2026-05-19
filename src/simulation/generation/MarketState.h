#pragma once
#include <array>

struct MarketState {
    double mid = 100.0;
    double drift = 0.0;
    double sigma = 0.2;
    double dt = 0.0001;
    double b = 0.001;
    int eventsPerTick = 10;
    std::array<double, 2> addCancelModOdds{55.0, 45.0};
};
