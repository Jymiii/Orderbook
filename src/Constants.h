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
    auto constexpr inline InvalidPrice = std::numeric_limits<Price>::quiet_NaN();
    TimeOfDay constexpr inline MarketCloseTime{16, 0, 0};}
#endif //ORDERBOOK_CONSTANTS_H
