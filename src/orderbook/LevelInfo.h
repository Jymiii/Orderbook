//
// Created by Jimi van der Meer on 10/02/2026.
//

#ifndef ORDERBOOK_LEVELINFO_H
#define ORDERBOOK_LEVELINFO_H

#include "Usings.h"

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

#endif //ORDERBOOK_LEVELINFO_H
