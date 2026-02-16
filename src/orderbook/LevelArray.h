#ifndef LEVEL_ARRAY_H
#define LEVEL_ARRAY_H

#include "Side.h"
#include "LevelData.h"

#include <array>
#include <cassert>
#include <stdexcept>
#include <unordered_map>
#include <utility>

template<Side S>
struct BestScanPolicy;

template<>
struct BestScanPolicy<Side::Sell> {
    static constexpr int start(int /*N*/) { return 0; }
    static constexpr int end(int N) { return N; }
    static constexpr int step = +1;
    static constexpr bool better(int a, int b) { return a <= b; }
};

template<>
struct BestScanPolicy<Side::Buy> {
    static constexpr int start(int N) { return N - 1; }
    static constexpr int end(int /*N*/) { return -1; }
    static constexpr int step = -1;
    static constexpr bool better(int a, int b) { return a >= b; }
};

template<int N, Side S>
class LevelArray {
    using P = BestScanPolicy<S>;

public:
    LevelArray() = default;
    LevelArray(const LevelArray&) = delete;
    LevelArray& operator=(const LevelArray&) = delete;

    Orders& getOrders(Price price) {
        const int idx = priceToIndex(price);
        if (idx < 0 || idx >= N) throw std::logic_error("Didnt resize in time");
        return levels_[idx];
    }

    const Orders& getOrders(Price price) const {
        const int idx = priceToIndex(price);
        if (idx < 0 || idx >= N) throw std::logic_error("Didnt resize in time");
        return levels_[idx];
    }

    std::pair<Price, Orders&> getBestOrders() {
        if (empty_) throw std::logic_error("side empty");
        assert(bestIdx_ >= 0 && bestIdx_ < N);
        return { indexToPrice(bestIdx_), levels_[bestIdx_] };
    }

    std::pair<Price, const Orders&> getBestOrders() const {
        if (empty_) throw std::logic_error("side empty");
        assert(bestIdx_ >= 0 && bestIdx_ < N);
        return { indexToPrice(bestIdx_), levels_[bestIdx_] };
    }

    std::pair<Price, Orders&> getWorstOrders() {
        if (empty_) throw std::logic_error("side empty");
        assert(worstIdx_ >= 0 && worstIdx_ < N);
        return { indexToPrice(worstIdx_), levels_[worstIdx_] };
    }

    std::pair<Price, const Orders&> getWorstOrders() const {
        if (empty_) throw std::logic_error("side empty");
        assert(worstIdx_ >= 0 && worstIdx_ < N);
        return { indexToPrice(worstIdx_), levels_[worstIdx_] };
    }

    [[nodiscard]] Price getBestPrice() const noexcept {
        return empty_ ? Price{} : indexToPrice(bestIdx_);
    }

    [[nodiscard]] Price getWorstPrice() const {
        if (empty_) throw std::logic_error("side empty");
        return indexToPrice(worstIdx_);
    }

    [[nodiscard]] bool empty() const noexcept { return empty_; }

    void onOrderAdded(Price price) {
        const int idx = priceToIndex(price);

        if (idx < 0 || idx >= N) throw std::logic_error("Didnt resize in time");

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
        if (idx < 0 || idx >= N) throw std::logic_error("Didnt resize in time");

        const bool removedBest  = (idx == bestIdx_);
        const bool removedWorst = (idx == worstIdx_);

        if (!removedBest && !removedWorst) return;

        if (removedBest)  updateBestIdx();
        if (removedWorst) updateWorstIdx();
    }

    bool canFullyFill(const std::unordered_map<Price, LevelData>& levelDatas,
                      Price limitPrice,
                      Quantity quantity) const
    {
        if (empty_) return false;

        const int limitIdx = priceToIndex(limitPrice);

        for (int i = bestIdx_;; i += P::step) {
            if (!levels_[i].empty()) {
                if (!P::better(i, limitIdx)) break;

                const Price p = indexToPrice(i);
                auto it = levelDatas.find(p);
                if (it != levelDatas.end()) {
                    const Quantity lvlQty = it->second.quantity_;
                    if (lvlQty >= quantity) return true;
                    quantity -= lvlQty;
                }
            }

            if (i == worstIdx_) break;
        }

        return false;
    }

    template <class F>
    void forEachLevelBestToWorst(F&& f) const {
        if (empty_) return;

        for (int i = bestIdx_;; i += P::step) {
            if (!levels_[i].empty()) {
                f(indexToPrice(i), levels_[i]);
            }
            if (i == worstIdx_) break;
        }
    }

private:
    void updateBestIdx() {
        for (int i = bestIdx_;; i += P::step) {
            if (!levels_[i].empty()) {
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
            if (!levels_[i].empty()) {
                worstIdx_ = i;
                empty_ = false;
                return;
            }
            if (i == bestIdx_) break;
        }

        empty_ = true;
        bestIdx_ = worstIdx_ = P::start(N);
    }

    [[nodiscard]] int priceToIndex(Price price) const {
        return static_cast<int>(price);
    }

    [[nodiscard]] Price indexToPrice(int idx) const {
        return static_cast<Price>(idx);
    }

private:
    std::array<Orders, N> levels_{};

    int  bestIdx_{P::start(N)};
    int  worstIdx_{P::start(N)};
    bool empty_{true};

    Price lowerBound_{0};
    Price upperBound_{static_cast<Price>(N - 1)};
};

#endif
