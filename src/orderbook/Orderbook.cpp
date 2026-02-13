//
// Created by Jimi van der Meer on 10/02/2026.
//

#include "Orderbook.h"

// ===== Lifecycle / background thread =====
void Orderbook::pruneStaleFillOrKill(std::vector<Level> &levels) {
    if (levels.empty()) return;

    auto &[_, orders] {*levels.rbegin()};
    auto &order = orders.front();
    if (order.getType() == OrderType::FillAndKill) {
        cancelOrderInternal(order.getId());
    } else if (order.getType() == OrderType::FillOrKill) {
        throw std::logic_error("There was a stale FOK order, should never be possible.");
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

Trades Orderbook::addOrder(Order order) {
    std::scoped_lock _{orderMutex_};
    return addOrderInternal(std::move(order));
}

void Orderbook::cancelOrder(OrderId orderId) {
    std::scoped_lock _{orderMutex_};
    cancelOrderInternal(orderId);
}

Trades Orderbook::modifyOrder(OrderModify orderModify) {
    std::scoped_lock _{orderMutex_};
    if (!orders_.contains(orderModify.getId())) return {};
    auto &ordersIterator = orders_.at(orderModify.getId());
    OrderType type{ordersIterator->getType()};
    cancelOrderInternal(orderModify.getId());
    return addOrderInternal(orderModify.toOrder(type));
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

    if (side == Side::Sell) {
        auto levelIt = get(price, asks_, std::greater<>());
        if (levelIt != asks_.end()) {
            levelIt->orders_.erase(listIt);
            if (levelIt->orders_.empty()) asks_.erase(levelIt);
        }
    } else {
        auto levelIt = get(price, bids_, std::less<>());
        if (levelIt != bids_.end()) {
            levelIt->orders_.erase(listIt);
            if (levelIt->orders_.empty()) bids_.erase(levelIt);
        }
    }
    orders_.erase(it);
}

Trades Orderbook::addOrderInternal(Order order) {
    if (order.getRemainingQuantity() == 0 || orders_.contains(order.getId())) {
        return {};
    }

    if (order.getType() == OrderType::Market) {
        if (order.getSide() == Side::Sell && !bids_.empty()) {
            const auto &[worstBidPrice, orders] = *bids_.begin();
            order.toGoodTillCancel(worstBidPrice);
        } else if (order.getSide() == Side::Buy && !asks_.empty()) {
            const auto &[worstAskPrice, orders] = *asks_.begin();
            order.toGoodTillCancel(worstAskPrice);
        } else return {};
    }

    if (order.getType() == OrderType::FillAndKill && !canMatch(order.getSide(), order.getPrice())) {
        return {};
    }

    if (order.getType() == OrderType::FillOrKill) {
        if (!canFullyFill(order.getSide(), order.getPrice(), order.getRemainingQuantity())) {
            return {};
        }
    }

    Level &level =
            (order.getSide() == Side::Buy) ? insertOrGet(order.getPrice(), bids_, std::less<>()) : insertOrGet(order.getPrice(), asks_, std::greater<>());
    Orders& orders = level.orders_;

    orders.push_back(std::move(order));
    auto iterator = std::prev(orders.end());

    orders_.emplace(iterator->getId(), iterator);

    onOrderAdded(*iterator);

    return matchOrders();
}

// ===== Matching / eligibility =====

bool Orderbook::canMatch(Side side, Price price) {
    if (price == Constants::INVALID_PRICE) return true;

    if (side == Side::Sell) {
        if (bids_.empty()) return false;
        const auto &[highestBid, _] = *(bids_.rbegin());
        return highestBid >= price;
    } else {
        if (asks_.empty()) return false;
        const auto &[lowestAsk, _] = *(asks_.rbegin());
        return lowestAsk <= price;
    }
}

bool Orderbook::canFullyFill(Side side, Price price, Quantity quantity) {
    if (side == Side::Sell) {
        return canFullyFill(bids_, price, quantity, std::greater_equal<>());
    } else {
        return canFullyFill(asks_, price, quantity, std::less_equal<>());
    }
}


Trades Orderbook::matchOrders() {
    if (bids_.empty() || asks_.empty()) return {};

    Trades trades;

    while (true) {
        if (bids_.empty() || asks_.empty()) {
            break;
        }
        auto bidIt = bids_.rbegin();
        auto askIt = asks_.rbegin();
        auto &[highestBid, bidOrders] = *bidIt;
        auto &[lowestAsk, askOrders] = *askIt;

        if (highestBid < lowestAsk) break;

        while (!askOrders.empty() && !bidOrders.empty()) {
            auto &bidOrder{bidOrders.front()};
            auto &askOrder{askOrders.front()};

            Quantity tradedQuantity = std::min(bidOrder.getRemainingQuantity(), askOrder.getRemainingQuantity());

            bidOrder.fill(tradedQuantity);
            askOrder.fill(tradedQuantity);

            trades.emplace_back(
                    bidOrder.getId(), askOrder.getId(),
                    bidOrder.getPrice(), askOrder.getPrice(),
                    tradedQuantity);

            const bool bidFilled = bidOrder.isFilled();
            const bool askFilled = askOrder.isFilled();

            if (bidFilled) {
                orders_.erase(bidOrder.getId());
                bidOrders.pop_front();
            }
            if (askFilled) {
                orders_.erase(askOrder.getId());
                askOrders.pop_front();
            }

            onOrderMatched(bidOrder.getPrice(), tradedQuantity, bidFilled);
            onOrderMatched(askOrder.getPrice(), tradedQuantity, askFilled);

        }
        if (bidOrders.empty()) { bids_.pop_back(); }
        if (askOrders.empty()) asks_.pop_back();
    }
    pruneStaleFillOrKill(bids_);
    pruneStaleFillOrKill(asks_);
    return trades;
}

// ===== Read-only views =====

[[nodiscard]] OrderbookLevelInfos Orderbook::getOrderInfos() const {

    std::scoped_lock _{orderMutex_};

    LevelInfos levelInfosBids, levelInfosAsks;
    auto createLevelInfo = [](Price price, const Orders& orders) {
        return LevelInfo{price, std::accumulate(orders.begin(), orders.end(), Quantity{0},
                                                [](Quantity quantity, const Order &order) {
                                                    return quantity + order.getRemainingQuantity();
                                                })};
    };
    auto bidsIt = bids_.rbegin();
    while (bidsIt != bids_.rend()) {
        const auto &[price, orders] = *bidsIt;
        levelInfosBids.push_back(createLevelInfo(price, orders));
        bidsIt = std::next(bidsIt);

    }
    auto asksIt = asks_.rbegin();
    while (asksIt != asks_.rend()) {
        const auto &[price, orders] = *asksIt;
        levelInfosAsks.push_back(createLevelInfo(price, orders));
        asksIt = std::next(asksIt);
    }

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
