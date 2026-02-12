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
#include <numeric>
#include <map>
#include <iostream>
#include <chrono>
#include <ctime>
#include <unistd.h>
#include <thread>
#include <mutex>

class Orderbook {
private:
    struct OrderEntry {
        OrderPtr order_{nullptr};
        OrderPtrs::iterator pos_;
    };

    struct LevelData {
        Quantity quantity_{};
        Quantity count_{};

        enum class Action {
            Add,
            Remove,
            Match
        };
    };

    std::unordered_map<Price, LevelData> levelData_;
    std::map<Price, OrderPtrs, std::greater<>> bids_;
    std::map<Price, OrderPtrs, std::less<>> asks_;
    std::unordered_map<OrderId, OrderEntry> orders_;

    mutable std::mutex orderMutex_{};
    std::thread gfdPruneThread_;
    bool shutdown_{false};
    std::condition_variable shutdownConditionVariable_{};

    friend class PruneTestHelper;

    template<typename T>
    void pruneStaleFillOrKill(std::map<Price, OrderPtrs, T> &orderMap) {
        if (orderMap.empty()) return;

        auto &[_, orders] {*orderMap.begin()};
        auto &order = orders.front();
        if (order->getType() == OrderType::FillAndKill) {
            cancelOrderInternal(order->getId());
        }
        else if (order->getType() == OrderType::FillOrKill) {
            throw std::logic_error("There was a stale FOK order, should never be possible.");
        }
    }

    void onOrderMatched(Price price, Quantity quantity, bool fullMatch);

    void onOrderAdded(const OrderPtr &order);

    void onOrderCanceled(const OrderPtr &order);

    void updateLevelData(Price price, Quantity quantity, LevelData::Action);

    bool canMatch(Side side, Price price);

    Trades matchOrders();

    void cancelOrders(const OrderIds &orderIds);

    void cancelOrderInternal(OrderId orderId);

    void pruneStaleGoodForDay();

    bool waitTillPruneTime();

    void pruneStaleGoodForNow();

    Trades addOrderInternal(const OrderPtr &sharedPtr);


public:
    explicit Orderbook(bool startPruneThread = true);
    Orderbook(const Orderbook&) = delete;
    Orderbook(Orderbook&&) = delete;
    Orderbook operator=(const Orderbook& other) = delete;
    Orderbook operator=(Orderbook&& other) = delete;
    ~Orderbook();

    Trades addOrder(const OrderPtr &order);

    Trades modifyOrder(OrderModify orderModify);

    void cancelOrder(OrderId orderId);

    [[nodiscard]] std::size_t size() const;

    [[nodiscard]] OrderbookLevelInfos getOrderInfos() const;

    bool canFullyFill(Side side, Price price, Quantity quantity);

    friend std::ostream &operator<<(std::ostream &os, Orderbook &ob) {
        return os << ob.getOrderInfos();
    }
};

#endif //ORDERBOOK_ORDERBOOK_H