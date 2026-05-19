#pragma once
#include <cstdint>

enum class OrderType : uint8_t
{
    GoodTillCancel,
    FillAndKill,
    Market,
    GoodForDay,
    FillOrKill,
    Size
};
