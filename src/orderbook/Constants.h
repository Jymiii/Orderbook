//
// Created by Jimi van der Meer on 10/02/2026.
//

#ifndef ORDERBOOK_CONSTANTS_H
#define ORDERBOOK_CONSTANTS_H

#include <numeric>
#include "Usings.h"

namespace Constants {
    struct TimeOfDay {
        int hour;
        int minute;
        int second;
    };

    auto constexpr inline INVALID_PRICE = std::numeric_limits<Price>::min();
    auto constexpr inline TICK_MULTIPLIER = 100;
    size_t constexpr inline LEVELARRAY_SIZE = 60000;
    size_t constexpr inline INITIAL_ORDER_CAPACITY = 200'000;
    TimeOfDay constexpr inline MarketCloseTime{16, 30, 00};
}
#endif //ORDERBOOK_CONSTANTS_H