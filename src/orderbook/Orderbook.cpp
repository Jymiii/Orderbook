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
    orders_.reserve(Constants::INITIAL_ORDER_CAPACITY);
}

Orderbook::~Orderbook() {
    {
        std::scoped_lock lock(orderMutex_);
        shutdown_ = true;
    }
    shutdownConditionVariable_.notify_all();
    if (gfdPruneThread_.joinable()) gfdPruneThread_.join();

#ifdef ORDERBOOK_ENABLE_INSTRUMENTATION
    if (addCount_ > 0)
        std::cout << "Average time for an add: " << addTotalTime_ / addCount_ * 1e9 << "ns {Total time spent: "
                  << addTotalTime_ << ", Count: " << addCount_ << "}\n";
    if (cancelCount_ > 0)
        std::cout << "Average time for a cancel: " << cancelTotalTime_ / cancelCount_ * 1e9 << "ns {Total time spent: "
                  << cancelTotalTime_ << ", Count: " << cancelCount_ << "}\n";
#endif
}


// ===== Public API =====

[[nodiscard]] std::size_t Orderbook::size() const {
    std::scoped_lock _{orderMutex_};
    return orders_.size();
}

void Orderbook::addOrder(Order order) {
#ifdef ORDERBOOK_ENABLE_INSTRUMENTATION
    addCount_++;
    timer_.start();
#endif
    std::scoped_lock _{orderMutex_};
    addOrderInternal(order);
#ifdef ORDERBOOK_ENABLE_INSTRUMENTATION
    addTotalTime_ += timer_.elapsed();
#endif
}

void Orderbook::cancelOrder(OrderId orderId) {
#ifdef ORDERBOOK_ENABLE_INSTRUMENTATION
    cancelCount_++;
    timer_.start();
#endif
    std::scoped_lock _{orderMutex_};
    cancelOrderInternal(orderId);
#ifdef ORDERBOOK_ENABLE_INSTRUMENTATION
    cancelTotalTime_ += timer_.elapsed();
#endif
}

void Orderbook::modifyOrder(OrderModify orderModify) {
#ifdef ORDERBOOK_ENABLE_INSTRUMENTATION
    modifyCount_++;
    timer_.start();
#endif
    std::scoped_lock _{orderMutex_};
    auto ordersIterator = orders_.find(orderModify.getId());
    if (ordersIterator == orders_.end()) return;
#ifdef ORDERBOOK_ENABLE_INSTRUMENTATION
    modifyWentThroughCount_++;
#endif
    OrderType type{ordersIterator->second->getType()};
    cancelOrderInternal(orderModify.getId());
    addOrderInternal(orderModify.toOrder(type));
#ifdef ORDERBOOK_ENABLE_INSTRUMENTATION
    modifyTotalTime_ += timer_.elapsed();
#endif
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
    if (it == orders_.end()) [[unlikely]] return;

    auto listIt = it->second;

    onOrderCanceled(*listIt);

    const Price price = listIt->getPrice();
    const Side side = listIt->getSide();

    if (side == Side::Buy) {
        bids_.getOrders(price).erase(listIt);
        bids_.onOrderRemoved(price);
    } else {
        asks_.getOrders(price).erase(listIt);
        asks_.onOrderRemoved(price);
    }
    orders_.erase(it);
}

void Orderbook::addOrderInternal(Order order) {
    if (order.getRemainingQuantity() == 0 || orders_.contains(order.getId())) [[unlikely]] {
        return;
    }

    const Side side = order.getSide();

    if (order.getType() == OrderType::Market) [[unlikely]] {
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

    if (order.getType() == OrderType::FillOrKill && !canFullyFill(side, price, order.getRemainingQuantity())) {
        return;
    }

    Orders &orders =
            (side == Side::Buy) ? bids_.getOrders(price) : asks_.getOrders(price);

    orders.push_back(order);
    auto iterator = std::prev(orders.end());

    orders_.emplace(order.getId(), iterator);

    onOrderAdded(order);

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
        return bids_.canFullyFill(price, quantity);
    } else {
        return asks_.canFullyFill(price, quantity);
    }
}


void Orderbook::matchOrders() {
    while (!bids_.empty() && !asks_.empty()) {
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
            trades_.emplace_back(
                    bidOrder.getId(), askOrder.getId(),
                    bidOrderPrice, askOrderPrice,
                    tradedQuantity);

            const bool bidFilled = bidOrder.isFilled();
            const bool askFilled = askOrder.isFilled();

            onOrderMatched(bidOrderPrice, tradedQuantity, bidFilled, Side::Buy);
            onOrderMatched(askOrderPrice, tradedQuantity, askFilled, Side::Sell);

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

void Orderbook::onOrderMatched(Price price, Quantity quantity, bool fullMatch, Side side) {
    if (fullMatch) {
        updateLevelData(price, quantity, LevelData::Action::Remove, side);
    } else {
        updateLevelData(price, quantity, LevelData::Action::Match, side);
    }
}

void Orderbook::onOrderAdded(const Order &order) {
    updateLevelData(order.getPrice(), order.getRemainingQuantity(), LevelData::Action::Add, order.getSide());
}

void Orderbook::onOrderCanceled(const Order &order) {
    updateLevelData(order.getPrice(), order.getRemainingQuantity(), LevelData::Action::Remove, order.getSide());
}

void Orderbook::updateLevelData(Price price, Quantity quantity, LevelData::Action action, Side side) {
    LevelData &data = (side == Side::Buy) ? bids_.getLevelData(price) : asks_.getLevelData(price);

    if (action == LevelData::Action::Add) data.count++;
    else if (action == LevelData::Action::Remove) data.count--;

    if (action == LevelData::Action::Remove || action == LevelData::Action::Match) {
        data.quantity -= quantity;
    } else {
        data.quantity += quantity;
    }
}
