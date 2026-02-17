//
// Created by Jimi van der Meer on 10/02/2026.
//

#include "Orderbook.h"

template<int N, Side S>
void Orderbook::pruneStaleFillOrKill(LevelArray<N, S> &levelArray) {
    if (levelArray.empty()) return;

    [[maybe_unused]] auto [bestPrice, orders] = levelArray.getBestOrders();

    auto &order = orders.front();
    switch (order.getType()) {
        case OrderType::FillAndKill:cancelOrderInternal(order.getId());
            break;

        case OrderType::FillOrKill:throw std::logic_error("There was a stale FOK order, should never be possible.");

        default:break;
    }
}

void Orderbook::pruneStaleGoodForDay() {
    while (true) {
        if (waitTillPruneTime()) {
            return;
        }
        pruneStaleGoodForNow();
    }
}

void Orderbook::pruneStaleGoodForNow() {
    OrderIds stale;
    {
        std::scoped_lock lock{orderMutex_};
        for (const auto &[id, ordersIterator]: orders_) {
            if (ordersIterator->getType() == OrderType::GoodForDay)
                stale.push_back(id);
        }
    }
    cancelOrders(stale);
}

bool Orderbook::waitTillPruneTime() {
    using namespace std::chrono;

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
        std::unique_lock<std::mutex> lock(orderMutex_);

        shutdownConditionVariable_.wait_for(
                lock, until,
                [&] { return shutdown_; }
        );

        if (shutdown_) return true;
    }
    return false;
}

Orderbook::Orderbook(bool startPruneThread) {
    if (startPruneThread) {
        gfdPruneThread_ = std::thread([this] {
            pruneStaleGoodForDay();
        });
    }
    orders_.reserve(200000);
}

Orderbook::~Orderbook() {
    {
        std::scoped_lock lock(orderMutex_);
        shutdown_ = true;
    }
    shutdownConditionVariable_.notify_all();
    if (gfdPruneThread_.joinable()) gfdPruneThread_.join();
}


// ===== Public API =====

[[nodiscard]] std::size_t Orderbook::size() const {
    std::scoped_lock _{orderMutex_};
    return orders_.size();
}

void Orderbook::addOrder(Order order) {
    std::scoped_lock _{orderMutex_};
    addOrderInternal(order);
}

void Orderbook::cancelOrder(OrderId orderId) {
    std::scoped_lock _{orderMutex_};
    cancelOrderInternal(orderId);
}

void Orderbook::modifyOrder(OrderModify orderModify) {
    std::scoped_lock _{orderMutex_};
    if (!orders_.contains(orderModify.getId())) return;
    auto &ordersIterator = orders_.at(orderModify.getId());
    OrderType type{ordersIterator->getType()};
    cancelOrderInternal(orderModify.getId());
    addOrderInternal(orderModify.toOrder(type));
}
// ===== Internal cancel / add helpers =====

void Orderbook::cancelOrders(const OrderIds &orderIds) {
    std::scoped_lock _{orderMutex_};

    for (OrderId id: orderIds) {
        cancelOrderInternal(id);
    }
}

void Orderbook::cancelOrderInternal(OrderId orderId) {
    auto it = orders_.find(orderId);
    if (it == orders_.end()) return;

    auto listIt = it->second;

    onOrderCanceled(*listIt);

    const Price price = listIt->getPrice();
    const Side side = listIt->getSide();

    if (side == Side::Buy) {
        auto &orders = bids_.getOrders(price);
        orders.erase(listIt);
        bids_.onOrderRemoved(price);
    } else {
        auto &orders = asks_.getOrders(price);
        orders.erase(listIt);
        asks_.onOrderRemoved(price);
    }
    orders_.erase(it);
}

void Orderbook::addOrderInternal(Order order) {
    if (order.getRemainingQuantity() == 0 || orders_.contains(order.getId())) {
        return;
    }

    const Side side = order.getSide();

    if (order.getType() == OrderType::Market) {
        if (side == Side::Sell && !bids_.empty()) {
            const Price worstBidPrice = bids_.getWorstPrice();
            order.toFillAndKill(worstBidPrice);
        } else if (side == Side::Buy && !asks_.empty()) {
            const Price worstAskPrice = asks_.getWorstPrice();
            order.toFillAndKill(worstAskPrice);
        } else return;
    }

    const Price price = order.getPrice();

    if (order.getType() == OrderType::FillAndKill && !canMatch(side, price)) {
        return;
    }

    if (order.getType() == OrderType::FillOrKill) {
        if (!canFullyFill(side, price, order.getRemainingQuantity())) {
            return;
        }
    }

    Orders &orders =
            (side == Side::Buy) ? bids_.getOrders(price) : asks_.getOrders(price);

    orders.push_back(order);
    auto iterator = std::prev(orders.end());

    orders_.emplace(iterator->getId(), iterator);

    onOrderAdded(*iterator);

    if (side == Side::Buy) bids_.onOrderAdded(price);
    else asks_.onOrderAdded(price);

    matchOrders();
}

// ===== Matching / eligibility =====

bool Orderbook::canMatch(Side side, Price price) {
    if (price == Constants::INVALID_PRICE) return true;

    if (side == Side::Sell) {
        if (bids_.empty()) return false;
        const auto &[highestBid, _] = bids_.getBestOrders();
        return highestBid >= price;
    } else {
        if (asks_.empty()) return false;
        const auto &[lowestAsk, _] = asks_.getBestOrders();
        return lowestAsk <= price;
    }
}

bool Orderbook::canFullyFill(Side side, Price price, Quantity quantity) {
    if (side == Side::Sell) {
        return bids_.canFullyFill(levelData_, price, quantity);
    } else {
        return asks_.canFullyFill(levelData_, price, quantity);
    }
}


void Orderbook::matchOrders() {
    if (bids_.empty() || asks_.empty()) return;

    while (true) {
        if (bids_.empty() || asks_.empty()) {
            break;
        }
        auto [highestBid, bidOrders] = bids_.getBestOrders();
        auto [lowestAsk, askOrders] = asks_.getBestOrders();

        if (highestBid < lowestAsk) break;

        while (!askOrders.empty() && !bidOrders.empty()) {
            auto &bidOrder{bidOrders.front()};
            auto &askOrder{askOrders.front()};

            const Quantity tradedQuantity = std::min(bidOrder.getRemainingQuantity(), askOrder.getRemainingQuantity());

            bidOrder.fill(tradedQuantity);
            askOrder.fill(tradedQuantity);

            const Price askOrderPrice = askOrder.getPrice();
            const Price bidOrderPrice = bidOrder.getPrice();
            trades.emplace_back(

                    bidOrder.getId(), askOrder.getId(),
                    bidOrderPrice, askOrderPrice,
                    tradedQuantity);

            const bool bidFilled = bidOrder.isFilled();
            const bool askFilled = askOrder.isFilled();

            onOrderMatched(bidOrderPrice, tradedQuantity, bidFilled);
            onOrderMatched(askOrderPrice, tradedQuantity, askFilled);

            if (bidFilled) {
                orders_.erase(bidOrder.getId());
                bidOrders.pop_front();
                if (bidOrders.empty()) bids_.onOrderRemoved(bidOrderPrice);
            }
            if (askFilled) {
                orders_.erase(askOrder.getId());
                askOrders.pop_front();
                if (askOrders.empty()) asks_.onOrderRemoved(askOrderPrice);
            }
        }
    }
    pruneStaleFillOrKill(bids_);
    pruneStaleFillOrKill(asks_);
}

// ===== Read-only views =====

[[nodiscard]] OrderbookLevelInfos Orderbook::getOrderInfos() const {
    std::scoped_lock _{orderMutex_};

    LevelInfos levelInfosBids, levelInfosAsks;

    auto createLevelInfo = [](Price price, const Orders &orders) {
        const Quantity total = std::accumulate(
                orders.begin(), orders.end(), Quantity{0},
                [](Quantity q, const Order &o) { return q + o.getRemainingQuantity(); }
        );
        return LevelInfo{price, total};
    };

    bids_.forEachLevelBestToWorst([&](Price price, const Orders &orders) {
        levelInfosBids.push_back(createLevelInfo(price, orders));
    });

    asks_.forEachLevelBestToWorst([&](Price price, const Orders &orders) {
        levelInfosAsks.push_back(createLevelInfo(price, orders));
    });

    return {levelInfosBids, levelInfosAsks};
}


// ===== Event-Driven methods ======

void Orderbook::onOrderMatched(Price price, Quantity quantity, bool fullMatch) {
    if (fullMatch) {
        updateLevelData(price, quantity, LevelData::Action::Remove);
    } else {
        updateLevelData(price, quantity, LevelData::Action::Match);
    }
}

void Orderbook::onOrderAdded(const Order &order) {
    updateLevelData(order.getPrice(), order.getRemainingQuantity(), LevelData::Action::Add);
}

void Orderbook::onOrderCanceled(const Order &order) {
    updateLevelData(order.getPrice(), order.getRemainingQuantity(), LevelData::Action::Remove);
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
