#pragma once

#include "Types.h"

struct LevelData {
    Quantity quantity{};
    Quantity count{};

    enum class Action {
        Add,
        Remove,
        Match
    };
};
