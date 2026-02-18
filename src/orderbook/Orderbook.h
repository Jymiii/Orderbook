//
// Created by Jimi van der Meer on 10/02/2026.
//

#ifndef ORDERBOOK_ORDERBOOK_H
#define ORDERBOOK_ORDERBOOK_H

#include "Usings.h"
#include "Order.h"
#include "Trade.h"
#include "OrderModify.h"
#include "OrderbookLevelInfos.h"
#include "LevelArray.h"
#include <numeric>
#include <iostream>
#include <chrono>
#include <ctime>
#include <thread>
#include <mutex>

#ifdef ORDERBOOK_ENABLE_INSTRUMENTATION
#include "shared/Timer.h"
#endif

class Orderbook {
private:
#ifdef ORDERBOOK_ENABLE_INSTRUMENTATION
    std::uint64_t addCount_{};
    std::uint64_t cancelCount_{};
    std::uint64_t modifyCount_{};
    std::uint64_t modifyWentThroughCount_{};
    long double addTotalTime_{};
    long double cancelTotalTime_{};
    long double modifyTotalTime_{};
    Timer timer_{};
#endif

    LevelArray<Constants::LEVELARRAY_SIZE, Side::Buy> bids_;
    LevelArray<Constants::LEVELARRAY_SIZE, Side::Sell> asks_;
    std::unordered_map<OrderId, OrdersIterator> orders_;
    Trades trades_;

    mutable std::mutex orderMutex_{};
    std::thread gfdPruneThread_;
    bool shutdown_{false};
    std::condition_variable shutdownConditionVariable_{};

    friend class PruneTestHelper;

    void onOrderMatched(Price price, Quantity quantity, bool fullMatch, Side side);

    void onOrderAdded(const Order &order);

    void onOrderCanceled(const Order &order);

    void updateLevelData(Price price, Quantity quantity, LevelData::Action action, Side side);

    bool canMatch(Side side, Price price);

    void matchOrders();

    void cancelOrders(const OrderIds &orderIds);

    template<int N, Side S>
    void pruneStaleFillOrKill(LevelArray<N, S> &levels);

    void cancelOrderInternal(OrderId orderId);

    void pruneStaleGoodForDay();

    bool waitTillPruneTime();

    void pruneStaleGoodForNow();

    void addOrderInternal(Order order);

public:
    explicit Orderbook(bool startPruneThread = true);

    ~Orderbook();

    void addOrder(Order order);

    void modifyOrder(OrderModify orderModify);

    void cancelOrder(OrderId orderId);

    [[nodiscard]] std::size_t size() const;

    [[nodiscard]] OrderbookLevelInfos getOrderInfos() const;

    [[nodiscard]] bool canFullyFill(Side side, Price price, Quantity quantity);

    friend std::ostream &operator<<(std::ostream &os, const Orderbook &ob) {
        return os << ob.getOrderInfos();
    }

    const Trades &getTrades() const {
        std::scoped_lock _{orderMutex_};
        return trades_;
    }

    void clearTrades() {
        std::scoped_lock _{orderMutex_};
        trades_.clear();
    }
};

#endif //ORDERBOOK_ORDERBOOK_H