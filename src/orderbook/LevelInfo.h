#pragma once

#include "Types.h"
#include <ostream>
#include <vector>

struct LevelInfo {
    Price price;
    Quantity quantity;

    friend std::ostream &operator<<(std::ostream &os, const LevelInfo &info) {
        os << "price: " << info.price << " Quantity: " << info.quantity;
        return os;
    }
};

using LevelInfos = std::vector<LevelInfo>;

inline std::ostream &operator<<(std::ostream &os, const LevelInfos &infos) {
    for (auto &info: infos) {
        os << info << '\n';
    }
    return os;
}
