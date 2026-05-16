#pragma once

#include "Types.h"

class Trade {
public:
    Trade(OrderId bidId, OrderId askId, Price bidPrice, Price askPrice, Quantity quantity) noexcept
        : bidId_{bidId}, askId_{askId}, bidPrice_{bidPrice}, askPrice_{askPrice},
          quantity_{quantity} {
    }

    [[nodiscard]] OrderId getBidId() const noexcept {
        return bidId_;
    }

    [[nodiscard]] OrderId getAskId() const noexcept {
        return askId_;
    }

    [[nodiscard]] Price getBidPrice() const noexcept {
        return bidPrice_;
    }

    [[nodiscard]] Price getAskPrice() const noexcept {
        return askPrice_;
    }

    [[nodiscard]] Quantity getQuantity() const noexcept {
        return quantity_;
    }

    friend bool operator==(const Trade &a, const Trade &b) = default;

private:
    OrderId bidId_;
    OrderId askId_;
    Price bidPrice_;
    Price askPrice_;
    Quantity quantity_;
};

using Trades = std::vector<Trade>;
