//
// Created by Jimi van der Meer on 12/02/2026.
//

#ifndef ORDERBOOK_TESTHELPERS_H
#define ORDERBOOK_TESTHELPERS_H
#include "orderbook/Orderbook.h"
#include <gtest/gtest.h>
struct OrderFactory {
    OrderId id = 0;

    OrderPtr make(OrderType type, Side side, Price price, Quantity qty) {
        return std::make_shared<Order>(id++, type, side, price, qty);
    }

    OrderPtr make(OrderId fixedId, OrderType type, Side side, Price price, Quantity qty) {
        return std::make_shared<Order>(fixedId, type, side, price, qty);
    }
    OrderPtr make(OrderId fixedId, Side side, Quantity qty) {
        return std::make_shared<Order>(fixedId, side, qty);
    }
};

inline bool hasTradeLike(const Trades &trades, const Trade &trade) {
    return std::ranges::any_of(trades,
                               [&trade](const Trade &other) {
                                   return other == trade;
                               });
}
#endif //ORDERBOOK_TESTHELPERS_H
