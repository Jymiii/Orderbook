#ifndef ORDERBOOK_LEVELARRAY_H
#define ORDERBOOK_LEVELARRAY_H

#include "Side.h"
#include "LevelData.h"

#include <array>
#include <cassert>
#include <functional>
#include <utility>
#include <optional>

template<Side S>
struct BestScanPolicy;

template<>
struct BestScanPolicy<Side::Sell> {
    static constexpr int start(int) { return 0; }

    static constexpr int end(int N) { return N; }

    static constexpr int step = +1;

    static constexpr bool better(int a, int b) { return a <= b; }
};

template<>
struct BestScanPolicy<Side::Buy> {
    static constexpr int start(int N) { return N - 1; }

    static constexpr int end(int) { return -1; }

    static constexpr int step = -1;

    static constexpr bool better(int a, int b) { return a >= b; }
};

template<int N, Side S>
class LevelArray {
    using P = BestScanPolicy<S>;

public:
    LevelArray() = default;

    LevelArray(const LevelArray &) = delete;

    LevelArray &operator=(const LevelArray &) = delete;

    std::optional<std::reference_wrapper<Orders> > getOrders(Price price) {
        const int idx = priceToIndex(price);
        assert(idx >= 0 && idx < N && "Price out of LevelArray range");
        if (idx < 0 || idx >= N) [[unlikely]] return std::nullopt;
        return levels_[idx].orders;
    }

    [[nodiscard]] std::optional<std::reference_wrapper<const Orders> > getOrders(Price price) const {
        const int idx = priceToIndex(price);
        assert(idx >= 0 && idx < N && "Price out of LevelArray range");
        if (idx < 0 || idx >= N) [[unlikely]] return std::nullopt;
        return levels_[idx].orders;
    }

    std::optional<std::reference_wrapper<LevelData> > getLevelData(Price price) {
        const int idx = priceToIndex(price);
        assert(idx >= 0 && idx < N && "Price out of LevelArray range");
        if (idx < 0 || idx >= N) [[unlikely]] return std::nullopt;
        return levels_[idx].data;
    }

    [[nodiscard]] std::optional<std::reference_wrapper<const LevelData> > getLevelData(Price price) const {
        const int idx = priceToIndex(price);
        assert(idx >= 0 && idx < N && "Price out of LevelArray range");
        if (idx < 0 || idx >= N) [[unlikely]] return std::nullopt;
        return levels_[idx].data;
    }

    std::optional<std::pair<Price, std::reference_wrapper<Orders> > > getBestOrders() {
        if (empty_) [[unlikely]] return std::nullopt;
        assert(bestIdx_ >= 0 && bestIdx_ < N);
        return std::pair<Price, std::reference_wrapper<Orders> >{indexToPrice(bestIdx_), levels_[bestIdx_].orders};
    }

    [[nodiscard]] std::optional<std::pair<Price, std::reference_wrapper<const Orders> > > getBestOrders() const {
        if (empty_) [[unlikely]] return std::nullopt;
        assert(bestIdx_ >= 0 && bestIdx_ < N);
        return std::pair<Price, std::reference_wrapper<const Orders> >{
            indexToPrice(bestIdx_), levels_[bestIdx_].orders
        };
    }

    [[nodiscard]] std::optional<Price> getBestPrice() const noexcept {
        if (empty_) [[unlikely]] return std::nullopt;
        assert(bestIdx_ >= 0 && bestIdx_ < N);
        return indexToPrice(bestIdx_);
    }

    std::optional<std::pair<Price, std::reference_wrapper<Orders> > > getWorstOrders() {
        if (empty_) [[unlikely]] return std::nullopt;
        assert(worstIdx_ >= 0 && worstIdx_ < N);
        return std::pair<Price, std::reference_wrapper<Orders> >{indexToPrice(worstIdx_), levels_[worstIdx_].orders};
    }

    [[nodiscard]] std::optional<std::pair<Price, std::reference_wrapper<const Orders> > > getWorstOrders() const {
        if (empty_) [[unlikely]] return std::nullopt;
        assert(worstIdx_ >= 0 && worstIdx_ < N);
        return std::pair<Price, std::reference_wrapper<const Orders> >{
            indexToPrice(worstIdx_), levels_[worstIdx_].orders
        };
    }

    [[nodiscard]] std::optional<Price> getWorstPrice() const noexcept {
        if (empty_) [[unlikely]] return std::nullopt;
        return indexToPrice(worstIdx_);
    }

    [[nodiscard]] bool empty() const noexcept { return empty_; }

    void onOrderAdded(Price price) {
        const int idx = priceToIndex(price);
        assert(idx >= 0 && idx < N && "Price out of LevelArray range");

        if (empty_) {
            bestIdx_ = worstIdx_ = idx;
            empty_ = false;
            return;
        }

        if (P::better(idx, bestIdx_)) bestIdx_ = idx;
        if (P::better(worstIdx_, idx)) worstIdx_ = idx;
    }

    void onOrderRemoved(Price price) {
        if (empty_) return;

        const int idx = priceToIndex(price);
        assert(idx >= 0 && idx < N && "Price out of LevelArray range");

        const bool removedBest = (idx == bestIdx_);
        const bool removedWorst = (idx == worstIdx_);

        if (removedBest) updateBestIdx();
        if (removedWorst) updateWorstIdx();
    }

    [[nodiscard]] bool canFullyFill(Price limitPrice, Quantity quantity) const {
        if (empty_) return false;

        const int limitIdx = priceToIndex(limitPrice);

        for (int i = bestIdx_;; i += P::step) {
            const auto &slot = levels_[i];
            if (!slot.orders.empty()) {
                if (!P::better(i, limitIdx)) break;

                const Quantity lvlQty = slot.data.quantity;
                if (lvlQty >= quantity) return true;
                quantity -= lvlQty;
            }

            if (i == worstIdx_) break;
        }

        return false;
    }

    template<class F>
    void forEachLevelBestToWorst(F &&f) const {
        if (empty_) return;

        for (int i = bestIdx_;; i += P::step) {
            if (!levels_[i].orders.empty()) {
                f(indexToPrice(i), levels_[i].orders);
            }
            if (i == worstIdx_) break;
        }
    }

private:
    void updateBestIdx() {
        for (int i = bestIdx_;; i += P::step) {
            if (!levels_[i].orders.empty()) {
                bestIdx_ = i;
                empty_ = false;
                return;
            }
            if (i == worstIdx_) break;
        }

        empty_ = true;
        bestIdx_ = worstIdx_ = P::start(N);
    }

    void updateWorstIdx() {
        constexpr int backStep = -P::step;

        for (int i = worstIdx_;; i += backStep) {
            if (!levels_[i].orders.empty()) {
                worstIdx_ = i;
                empty_ = false;
                return;
            }
            if (i == bestIdx_) break;
        }

        empty_ = true;
        bestIdx_ = worstIdx_ = P::start(N);
    }

    [[nodiscard]] static int priceToIndex(Price price) {
        return static_cast<int>(price);
    }

    [[nodiscard]] static Price indexToPrice(int idx) {
        return static_cast<Price>(idx);
    }

private:
    struct LevelSlot {
        Orders orders{};
        LevelData data{};
    };

    std::array<LevelSlot, N> levels_{};

    int bestIdx_{P::start(N)};
    int worstIdx_{P::start(N)};
    bool empty_{true};
};

#endif //ORDERBOOK_LEVELARRAY_H
