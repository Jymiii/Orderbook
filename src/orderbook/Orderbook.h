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
    std::vector<Level> bids_;
    std::vector<Level> asks_;
    std::unordered_map<OrderId, OrdersIterator> orders_;

    mutable std::mutex orderMutex_{};
    std::thread gfdPruneThread_;
    bool shutdown_{false};
    std::condition_variable shutdownConditionVariable_{};

    friend class PruneTestHelper;

    void onOrderMatched(Price price, Quantity quantity, bool fullMatch);

    void onOrderAdded(const Order &order);

    void onOrderCanceled(const Order &order);

    void updateLevelData(Price price, Quantity quantity, LevelData::Action);

    bool canMatch(Side side, Price price);

    Trades matchOrders();

    void cancelOrders(const OrderIds &orderIds);

    void pruneStaleFillOrKill(std::vector<Level> &levels);

    void cancelOrderInternal(OrderId orderId);

    void pruneStaleGoodForDay();

    bool waitTillPruneTime();

    void pruneStaleGoodForNow();

    Trades addOrderInternal(Order order);

    template <class T, class Compare>
    Level& insertOrGet(Price price, T& levels, Compare comp) {
        auto it = std::lower_bound(levels.begin(), levels.end(), price, [comp](const auto& p, Price price) {
            return comp(p.price_, price);
        });

        if (it == levels.end() || it->price_ != price){
            it = levels.insert(it, Level{price, {}});
        }
        return *it;
    }

    template <class T, class Compare>
    typename T::iterator get(Price price, T& levels, Compare comp) {
        auto it = std::lower_bound(levels.begin(), levels.end(), price, [comp](const auto& p, Price price) {
            return comp(p.price_, price);
        });

        assert(it != levels.end() && it->price_ == price);
        return it;
    }

    template<class T, class Compare>
    bool canFullyFill(T& levels, Price price, Quantity quantity, Compare comp) {
        if (levels.empty()) return false;
        auto it = levels.rbegin();
        while (it != levels.rend()) {
            const auto &[bestPrice, orders] = *it;
            auto levelQuantity = levelData_.at(bestPrice).quantity_;
            if (comp(bestPrice, price)) {
                if (quantity <= levelQuantity) return true;
                quantity -= levelQuantity;
            } else { break; }
            it = std::next(it);
        }
        return false;
    }

public:
    explicit Orderbook(bool startPruneThread = true);

    ~Orderbook();

    Trades addOrder(Order order);

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