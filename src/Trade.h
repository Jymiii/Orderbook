//
// Created by Jimi van der Meer on 10/02/2026.
//

#ifndef ORDERBOOK_TRADE_H
#define ORDERBOOK_TRADE_H
#include "Usings.h"
class Trade {
public:
    Trade(OrderId bidId, OrderId askId, Price bidPrice, Price askPrice, Quantity quantity)
            : bidId_{bidId}, askId_{askId}, bidPrice_{bidPrice}, askPrice_{askPrice},
              quantity_{quantity} {}

private:
    OrderId bidId_;
    OrderId askId_;
    Price bidPrice_;
    Price askPrice_;
    Quantity quantity_;
};

using Trades = std::vector<Trade>;
#endif //ORDERBOOK_TRADE_H
