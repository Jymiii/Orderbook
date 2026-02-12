//
// Created by Jimi van der Meer on 10/02/2026.
//

#ifndef ORDERBOOK_ORDERBOOKLEVELINFOS_H
#define ORDERBOOK_ORDERBOOKLEVELINFOS_H

#include <ostream>
#include "LevelInfo.h"

class OrderbookLevelInfos {
public:
    OrderbookLevelInfos(LevelInfos bids, LevelInfos asks)
            :
            bids_{std::move(bids)}, asks_{std::move(asks)} {}

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
#endif //ORDERBOOK_ORDERBOOKLEVELINFOS_H
