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
    auto constexpr inline INVALID_PRICE = std::numeric_limits<Price>::quiet_NaN();
    auto constexpr inline TICK_MULTIPLIER = 100;
    TimeOfDay constexpr inline MarketCloseTime{00, 00, 20};
}
#endif //ORDERBOOK_CONSTANTS_H
