#pragma once

#include "Types.h"
#include "Side.h"
#include "OrderType.h"
#include "Constants.h"
#include <cassert>
#include <list>
#include <ostream>
#include <utility>

class Order {
public:
    Order(OrderId id, OrderType type, Side side, Price price, Quantity quantity) noexcept
        : id_{id}, type_{type}, side_{side}, price_{price}, remainingQuantity_{quantity} {
    }

    Order(OrderId id, Side side, Quantity quantity) noexcept
        : Order(id, OrderType::Market, side, Constants::INVALID_PRICE, quantity) {
    }

    [[nodiscard]] OrderId getId() const noexcept {
        return id_;
    }

    [[nodiscard]] OrderType getType() const noexcept {
        return type_;
    }

    [[nodiscard]] Side getSide() const noexcept {
        return side_;
    }

    [[nodiscard]] Price getPrice() const noexcept {
        return price_;
    }

    [[nodiscard]] Quantity getRemainingQuantity() const noexcept {
        return remainingQuantity_;
    }

    [[nodiscard]] bool isFilled() const noexcept {
        return getRemainingQuantity() == 0;
    }

    void fill(Quantity quantity) noexcept {
        assert(quantity <= getRemainingQuantity());
        remainingQuantity_ -= quantity;
    }

    void toGoodTillCancel(Price price) noexcept {
        type_ = OrderType::GoodTillCancel;
        price_ = price;
    }

    void toFillAndKill(Price price) noexcept {
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
