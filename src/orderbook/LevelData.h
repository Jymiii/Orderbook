#pragma once

#include "Usings.h"

struct LevelData {
    Quantity quantity{};
    Quantity count{};

    enum class Action {
        Add,
        Remove,
        Match
    };
};
