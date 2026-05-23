#pragma once

#include <numeric>
#include "Types.h"

namespace Constants {
    struct TimeOfDay {
        int hour;
        int minute;
        int second;
    };

    auto constexpr inline INVALID_PRICE = std::numeric_limits<Price>::min();
    auto constexpr inline TICK_MULTIPLIER = 100;
    size_t constexpr inline LEVELARRAY_SIZE = 1<<17;
    size_t constexpr inline INITIAL_ORDER_CAPACITY = 200'000;
    TimeOfDay constexpr inline MarketCloseTime{16, 30, 00};
}
