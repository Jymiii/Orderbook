#ifndef ORDERBOOK_ORDERREGISTRY_H
#define ORDERBOOK_ORDERREGISTRY_H

#include "orderbook/Order.h"
#include <algorithm>
#include <optional>
#include <random>
#include <unordered_map>
#include <vector>

class OrderRegistry {
public:
    void onNew(const Order& o) {
        auto [it, inserted] = live_.emplace(o.getId(), o);
        if (!inserted) { it->second = o; return; }

        idToIndex_[o.getId()] = ids_.size();
        ids_.push_back(o.getId());
    }

    void onCancel(OrderId id) { erase(id); }

    void onModify(const Order& o) {
        auto it = live_.find(o.getId());
        if (it == live_.end()) return;
        it->second = o;
    }

    bool empty() const { return ids_.empty(); }

    std::optional<Order> randomLive(std::mt19937& rng) const {
        if (ids_.empty()) return std::nullopt;
        std::uniform_int_distribution<std::size_t> dist(0, ids_.size() - 1);

        OrderId id = ids_[dist(rng)];
        auto it = live_.find(id);
        if (it == live_.end()) return std::nullopt;

        return it->second;
    }

private:
    void erase(OrderId id) {
        auto liveIt = live_.find(id);
        if (liveIt == live_.end()) return;
        live_.erase(liveIt);

        auto idxIt = idToIndex_.find(id);
        if (idxIt == idToIndex_.end()) return;

        std::size_t idx = idxIt->second;
        std::size_t last = ids_.size() - 1;

        if (idx != last) {
            OrderId lastId = ids_[last];
            ids_[idx] = lastId;
            idToIndex_[lastId] = idx;
        }

        ids_.pop_back();
        idToIndex_.erase(idxIt);
    }

    std::unordered_map<OrderId, Order> live_;
    std::vector<OrderId> ids_;
    std::unordered_map<OrderId, std::size_t> idToIndex_;
};

#endif
