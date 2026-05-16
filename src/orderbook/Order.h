//
// Created by Jimi van der Meer on 10/02/2026.
//

#ifndef ORDERBOOK_ORDER_H
#define ORDERBOOK_ORDER_H

#include "Usings.h"
#include "Side.h"
#include "OrderType.h"
#include "Constants.h"
#include <cassert>
#include <iostream>
#include <list>

class Order {
public:
    Order(OrderId id, OrderType type, Side side, Price price, Quantity quantity)
        : id_{id}, type_{type}, side_{side}, price_{price}, remainingQuantity_{quantity} {
    }

    Order(OrderId id, Side side, Quantity quantity) : Order(id, OrderType::Market, side, Constants::INVALID_PRICE,
                                                            quantity) {
    }

    [[nodiscard]] OrderId getId() const {
        return id_;
    }

    [[nodiscard]] OrderType getType() const {
        return type_;
    }

    [[nodiscard]] Side getSide() const {
        return side_;
    }

    [[nodiscard]] Price getPrice() const {
        return price_;
    }

    [[nodiscard]] Quantity getRemainingQuantity() const {
        return remainingQuantity_;
    }

    [[nodiscard]] bool isFilled() const {
        return getRemainingQuantity() == 0;
    }

    void fill(Quantity quantity) {
        assert(quantity <= getRemainingQuantity());
        remainingQuantity_ -= quantity;
    }

    void toGoodTillCancel(Price price) {
        type_ = OrderType::GoodTillCancel;
        price_ = price;
    }

    void toFillAndKill(Price price) {
        type_ = OrderType::FillAndKill;
        price_ = price;
    }

    friend std::ostream &operator<<(std::ostream &os, const Order &order) {
        return os << order.id_ << "," << std::to_underlying(order.type_) << "," << std::to_underlying(order.side_)
               << "," << order.price_ << "," << order.remainingQuantity_ << "\n";
    }

private:
    OrderId id_{};
    OrderType type_;
    Side side_;
    Price price_{};
    Quantity remainingQuantity_{};
};

using Orders = std::list<Order>;
using OrdersIterator = std::list<Order>::iterator;
#endif //ORDERBOOK_ORDER_H