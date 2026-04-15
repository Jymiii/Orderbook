#pragma once

#include "Types.h"
#include "Side.h"
#include "OrderType.h"
#include "Order.h"

class OrderModify {
public:
    OrderModify(OrderId id, Side side, Price price, Quantity quantity)
        : id_{id}, side_{side}, price_{price}, quantity_{quantity} {
    }

    [[nodiscard]] OrderId getId() const {
        return id_;
    }

    [[nodiscard]] Side getSide() const {
        return side_;
    }

    [[nodiscard]] Price getPrice() const {
        return price_;
    }

    [[nodiscard]] Quantity getQuantity() const {
        return quantity_;
    }

    [[nodiscard]] Order toOrder(OrderType type) const {
        return {id_, type, side_, price_, quantity_};
    }

private:
    OrderId id_;
    Side side_;
    Price price_;
    Quantity quantity_;
};
