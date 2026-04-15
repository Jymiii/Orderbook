#pragma once

#include "LevelInfo.h"
#include <ostream>

class OrderbookLevelInfos {
public:
    OrderbookLevelInfos(LevelInfos bids, LevelInfos asks)
        : bids_{std::move(bids)}, asks_{std::move(asks)} {
    }

    [[nodiscard]] const LevelInfos &getBids() const { return bids_; }

    [[nodiscard]] const LevelInfos &getAsks() const { return asks_; }

    friend std::ostream &operator<<(std::ostream &os, const OrderbookLevelInfos &infos) {
        os << "bids_: " << infos.bids_ << "\n\n" << "asks_: " << infos.asks_;
        return os;
    }

private:
    LevelInfos bids_;
    LevelInfos asks_;
};
