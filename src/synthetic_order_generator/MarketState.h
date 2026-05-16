#pragma once

struct MarketState {
    double mid = 100.0;
    double drift = 0.1;
    double sigma = 0.2;
    double dt = 0.0001;
    double b = 0.001;
};
