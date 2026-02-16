//
// Created by Jimi van der Meer on 12/02/2026.
//

#ifndef ORDERBOOK_MARKETSTATE_H
#define ORDERBOOK_MARKETSTATE_H
struct MarketState {
    double mid = 100.0;
    double drift = 0.1;
    double sigma = 0.2;
    double dt = 0.0001;
    double b = 0.002;
};
#endif //ORDERBOOK_MARKETSTATE_H
