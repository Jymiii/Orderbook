//
// Created by Jimi van der Meer on 12/02/2026.
//
#include "orderbook/Order.h"
#ifndef ORDERBOOK_ORDEREVENT_H
#define ORDERBOOK_ORDEREVENT_H
enum class EventType { New, Cancel, Modify };

struct OrderEvent {
    EventType event;
    Order order;
};

#endif //ORDERBOOK_ORDEREVENT_H
