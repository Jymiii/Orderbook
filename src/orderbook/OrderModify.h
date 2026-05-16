//
// Created by Jimi van der Meer on 10/02/2026.
//

#ifndef ORDERBOOK_ORDERMODIFY_H
#define ORDERBOOK_ORDERMODIFY_H

#include "Usings.h"
#include "Side.h"
#include "OrderType.h"
#include "Order.h"

class OrderModify {
public:
    OrderModify(OrderId id, Side side, Price price, Quantity quantity) noexcept
        : id_{id}, side_{side}, price_{price}, quantity_{quantity} {
    }

    [[nodiscard]] OrderId getId() const noexcept {
        return id_;
    }

    [[nodiscard]] Side getSide() const noexcept {
        return side_;
    }

    [[nodiscard]] Price getPrice() const noexcept {
        return price_;
    }

    [[nodiscard]] Quantity getQuantity() const noexcept {
        return quantity_;
    }

    [[nodiscard]] Order toOrder(OrderType type) const noexcept {
        return {id_, type, side_, price_, quantity_};
    }

private:
    OrderId id_;
    Side side_;
    Price price_;
    Quantity quantity_;
};

#endif //ORDERBOOK_ORDERMODIFY_H
