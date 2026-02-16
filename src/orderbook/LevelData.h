//
// Created by Jimi van der Meer on 16/02/2026.
//

#ifndef ORDERBOOK_LEVELDATA_H
#define ORDERBOOK_LEVELDATA_H

#include "Usings.h"

struct LevelData {
    Quantity quantity_{};
    Quantity count_{};

    enum class Action {
        Add,
        Remove,
        Match
    };
};
#endif //ORDERBOOK_LEVELDATA_H
