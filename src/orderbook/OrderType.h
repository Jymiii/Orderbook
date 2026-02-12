//
// Created by Jimi van der Meer on 10/02/2026.
//

#ifndef ORDERBOOK_ORDERTYPE_H
#define ORDERBOOK_ORDERTYPE_H
enum class OrderType {
    GoodTillCancel,
    FillAndKill,
    Market,
    GoodForDay,
    FillOrKill
};
#endif //ORDERBOOK_ORDERTYPE_H
