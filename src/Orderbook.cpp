//
// Created by Jimi van der Meer on 10/02/2026.
//

#include "Orderbook.h"

// ===== Lifecycle / background thread =====

void Orderbook::pruneStaleGoodForDay() {
    using namespace std::chrono;

    while (true) {
        auto now = system_clock::now();
        std::time_t t = system_clock::to_time_t(now);

        std::tm tm{};
        localtime_r(&t, &tm);

        tm.tm_hour = Constants::MarketCloseTime.hour;
        tm.tm_min = Constants::MarketCloseTime.minute;
        tm.tm_sec = Constants::MarketCloseTime.second;

        auto close_tp = system_clock::from_time_t(std::mktime(&tm));
        if (close_tp <= now) {
            tm.tm_mday += 1;
            close_tp = system_clock::from_time_t(std::mktime(&tm));
        }
        auto until = close_tp - now;

        {
            std::unique_lock lock{orderMutex_};
            if (shutdown_.load(std::memory_order_acquire)
                || shutdownConditionVariable_.wait_for(lock, until) == std::cv_status::no_timeout) {
                return;
            }
        }

        OrderIds stale;
        {
            std::scoped_lock lock{orderMutex_};
            for (const auto &[id, orderPtr]: orders_) {
                if (orderPtr.order_->getType() == OrderType::GoodForDay)
                    stale.push_back(id);
            }
        }
        cancelOrders(stale);
    }
}

Orderbook::~Orderbook() {
    shutdown_.store(true, std::memory_order_release);
    shutdownConditionVariable_.notify_one();
    gfdPruneThread_.join();
}

// ===== Public API =====

[[nodiscard]] std::size_t Orderbook::size() const {
    std::scoped_lock _{orderMutex_};
    return orders_.size();
}

Trades Orderbook::addOrder(const OrderPtr &sharedPtr) {
    std::scoped_lock _{orderMutex_};
    return addOrderInternal(sharedPtr);
}

void Orderbook::cancelOrder(OrderId orderId) {
    std::scoped_lock _{orderMutex_};
    cancelOrderInternal(orderId);
}

Trades Orderbook::modifyOrder(OrderModify orderModify) {
    std::scoped_lock _{orderMutex_};
    if (!orders_.contains(orderModify.getId())) return {};
    auto &[existingOrder, it] = orders_.at(orderModify.getId());
    OrderType type{existingOrder->getType()};
    cancelOrderInternal(orderModify.getId());
    return addOrderInternal(orderModify.toOrderPtr(type));
}
// ===== Internal cancel / add helpers =====

void Orderbook::cancelOrders(const OrderIds &orderIds) {
    std::scoped_lock _{orderMutex_};

    for (OrderId id: orderIds) {
        cancelOrderInternal(id);
    }
}

void Orderbook::cancelOrderInternal(OrderId orderId) {
    if (!orders_.contains(orderId)) return;

    const auto [order, iterator] {orders_.at(orderId)};
    orders_.erase(order->getId());

    Price price = order->getPrice();
    if (order->getSide() == Side::Sell) {
        auto &orders = asks_.at(price);
        orders.erase(iterator);
        if (orders.empty()) {
            asks_.erase(price);
        }
    } else if (order->getSide() == Side::Buy) {
        auto &orders = bids_.at(price);
        orders.erase(iterator);
        if (orders.empty()) {
            bids_.erase(price);
        }
    }

    onOrderCanceled(order);
}

Trades Orderbook::addOrderInternal(const OrderPtr &order) {
    if (orders_.contains(order->getId())) {
        return {};
    }

    if (order->getType() == OrderType::Market) {
        if (order->getSide() == Side::Sell && !bids_.empty()) {
            const auto &[worstBidPrice, orders] = *bids_.rbegin();
            order->toGoodTillCancel(worstBidPrice);
        } else if (order->getSide() == Side::Buy && !asks_.empty()) {
            const auto &[worstAskPrice, orders] = *asks_.rbegin();
            order->toGoodTillCancel(worstAskPrice);
        } else return {};
    }

    if (order->getType() == OrderType::FillAndKill && !canMatch(order->getSide(), order->getPrice())) {
        return {};
    }

    if (order->getType() == OrderType::FillOrKill) {
        if (!canFullyFill(order->getSide(), order->getPrice(), order->getRemainingQuantity())) {
            return {};
        } else {
            order->toFillAndKill();
        }
    }

    OrderPtrs &orders =
            (order->getSide() == Side::Buy) ? bids_[order->getPrice()] : asks_[order->getPrice()];

    orders.push_back(order);
    auto iterator = std::prev(orders.end());

    orders_.insert({order->getId(), OrderEntry{order, iterator}});

    onOrderAdded(order);

    return matchOrders();
}

// ===== Matching / eligibility =====

bool Orderbook::canMatch(Side side, Price price) {
    if (price == Constants::InvalidPrice) return true;

    if (side == Side::Sell) {
        if (bids_.empty()) return false;
        const auto &[highestBid, _] = *(bids_.begin());
        return highestBid >= price;
    } else {
        if (asks_.empty()) return false;
        const auto &[lowestAsk, _] = *(asks_.begin());
        return lowestAsk <= price;
    }
}

bool Orderbook::canFullyFill(Side side, Price price, Quantity quantity) {
    if (side == Side::Sell) {
        if (bids_.empty()) return false;
        for (const auto &[bestBidPrice, orderPtrs]: bids_) {
            auto levelQuantity = levelData_.at(bestBidPrice).quantity_;
            if (bestBidPrice >= price) {
                if (quantity <= levelQuantity) return true;
                quantity -= levelQuantity;
            } else { break; }
        }
    } else {
        if (asks_.empty()) return false;
        for (const auto &[bestAskPrice, orderPtrs]: asks_) {
            auto levelQuantity = levelData_.at(bestAskPrice).quantity_;
            if (bestAskPrice <= price) {
                if (quantity <= levelQuantity) return true;
                quantity -= levelQuantity;
            } else { break; }
        }
    }
    return false;
}


Trades Orderbook::matchOrders() {
    if (bids_.empty() || asks_.empty()) return {};

    Trades trades;
    trades.reserve(orders_.size());

    while (true) {
        if (bids_.empty() || asks_.empty()) {
            break;
        }
        auto &[highestBid, bidOrders] = *(bids_.begin());
        auto &[lowestAsk, askOrders] = *(asks_.begin());

        if (highestBid < lowestAsk) break;

        while (!askOrders.empty() && !bidOrders.empty()) {
            auto bidOrder{bidOrders.front()};
            auto askOrder{askOrders.front()};

            Quantity tradedQuantity = std::min(bidOrder->getRemainingQuantity(), askOrder->getRemainingQuantity());

            bidOrder->fill(tradedQuantity);
            askOrder->fill(tradedQuantity);

            trades.emplace_back(
                    bidOrder->getId(), askOrder->getId(),
                    bidOrder->getPrice(), askOrder->getPrice(),
                    tradedQuantity);

            const bool bidFilled = bidOrder->isFilled();
            const bool askFilled = askOrder->isFilled();

            if (bidFilled) {
                orders_.erase(bidOrder->getId());
                bidOrders.pop_front();
            }
            if (askFilled) {
                orders_.erase(askOrder->getId());
                askOrders.pop_front();
            }

            onOrderMatched(bidOrder->getPrice(), tradedQuantity, bidFilled);
            onOrderMatched(askOrder->getPrice(), tradedQuantity, askFilled);

        }
        if (bidOrders.empty()) { bids_.erase(highestBid); }
        else { pruneStaleFillOrKill(bids_); }

        if (askOrders.empty()) asks_.erase(lowestAsk);
        else pruneStaleFillOrKill(asks_);
    }
    return trades;
}

// ===== Read-only views =====

[[nodiscard]] OrderbookLevelInfos Orderbook::getOrderInfos() const {

    std::scoped_lock _{orderMutex_};

    LevelInfos levelInfosBids, levelInfosAsks;
    auto createLevelInfo = [](Price price, const OrderPtrs &orders) {
        return LevelInfo{price, std::accumulate(orders.begin(), orders.end(), Quantity{0},
                                                [](Quantity quantity, const OrderPtr &order) {
                                                    return quantity + order->getRemainingQuantity();
                                                })};
    };

    for (const auto &[price, orders]: bids_) {
        levelInfosBids.push_back(createLevelInfo(price, orders));
    }
    for (const auto &[price, orders]: asks_) {
        levelInfosAsks.push_back(createLevelInfo(price, orders));
    }

    return {levelInfosBids, levelInfosAsks};
}

void Orderbook::onOrderMatched(Price price, Quantity quantity, bool fullMatch) {
    if (fullMatch) {
        updateLevelData(price, quantity, LevelData::Action::Remove);
    } else {
        updateLevelData(price, quantity, LevelData::Action::Match);
    }
}

void Orderbook::onOrderAdded(const OrderPtr &order) {
    updateLevelData(order->getPrice(), order->getRemainingQuantity(), LevelData::Action::Add);
}

void Orderbook::onOrderCanceled(const OrderPtr &order) {
    updateLevelData(order->getPrice(), order->getRemainingQuantity(), LevelData::Action::Remove);
}

void Orderbook::updateLevelData(Price price, Quantity quantity, LevelData::Action action) {
    auto &data = levelData_[price];
    if (action == LevelData::Action::Add) data.count_++;
    else if (action == LevelData::Action::Remove) data.count_--;

    if (action == LevelData::Action::Remove || action == LevelData::Action::Match) {
        data.quantity_ -= quantity;
    } else {
        data.quantity_ += quantity;
    }

    if (data.count_ == 0)
        levelData_.erase(price);
}
