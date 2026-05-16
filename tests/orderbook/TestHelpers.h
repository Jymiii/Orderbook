//
// Created by Jimi van der Meer on 12/02/2026.
//

#ifndef ORDERBOOK_TESTHELPERS_H
#define ORDERBOOK_TESTHELPERS_H

#include "orderbook/Orderbook.h"
#include <gtest/gtest.h>

struct OrderFactory {
    OrderId id = 0;

    Order make(OrderType type, Side side, Price price, Quantity qty) {
        return {id++, type, side, price, qty};
    }

    Order make(OrderId fixedId, OrderType type, Side side, Price price, Quantity qty) {
        return {fixedId, type, side, price, qty};
    }

    Order make(OrderId fixedId, Side side, Quantity qty) {
        return {fixedId, side, qty};
    }
};

inline bool hasTradeLike(const Trades &trades, const Trade &trade) {
    return std::ranges::any_of(trades,
                               [&trade](const Trade &other) {
                                   return other == trade;
                               });
}

class PruneTestHelper {
public:
    static void pruneStaleGoodForNow(Orderbook &ob) {
        ob.pruneStaleGoodForNow();
    }
};

#endif //ORDERBOOK_TESTHELPERS_H
