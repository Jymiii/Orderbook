//#include <vector>
//#include <random>
//#include <chrono>
//#include <iostream>
//#include <algorithm>
//#include <cmath>
//#include <climits>
//
//#include "orderbook/Orderbook.h"
//#include "orderbook/Order.h"
//#include "orderbook/OrderModify.h"
//#include "orderbook/Trade.h"
//
//// ANSI color codes
//#define COL_GREEN   "\033[32m"
//#define COL_YELLOW  "\033[33m"
//#define COL_MAGENTA "\033[35m"
//#define COL_RED     "\033[31m"
//#define COL_RESET   "\033[0m"
//
//constexpr int BENCH_RUNS  = 1'000'000;
//constexpr int RANGE_LOW   = 100'000;
//
//const std::vector<unsigned long long> histogram_edges = { 50, 100, 200, ULLONG_MAX };
//
//// --- Stats helpers ---
//static void print_stats(const std::vector<unsigned long long>& lat) {
//    if (lat.empty()) return;
//
//    std::vector<unsigned long long> sorted = lat;
//    std::sort(sorted.begin(), sorted.end());
//
//    auto p50  = sorted[sorted.size() / 2];
//    auto p99  = sorted[static_cast<size_t>(sorted.size() * 0.99)];
//    auto p999 = sorted[static_cast<size_t>(sorted.size() * 0.999)];
//    auto mx   = sorted.back();
//
//    std::cout << "Latency (steady_clock ticks): p50 " << p50
//              << ", p99 " << p99
//              << ", p99.9 " << p999
//              << ", max " << mx << "\n";
//}
//
//static void print_histogram(const std::vector<unsigned long long>& lat) {
//    std::vector<int> bins(histogram_edges.size(), 0);
//    for (unsigned long long x : lat) {
//        for (int i = 0; i < (int)histogram_edges.size(); ++i) {
//            if (x < histogram_edges[i]) { bins[i]++; break; }
//        }
//    }
//
//    const char* colors[] = {COL_GREEN, COL_YELLOW, COL_MAGENTA, COL_RED};
//    const char* labels[] = {"0-50", "50-100", "100-200", "200+"};
//
//    for (int i = 0; i < (int)bins.size(); ++i) {
//        int bar_len = bins[i] > 0 ? 1 + int(30.0 * bins[i] / lat.size()) : 0;
//        std::cout << "[ " << colors[i];
//        for (int j = 0; j < bar_len; ++j) std::cout << "■";
//        std::cout << COL_RESET << std::string(33 - bar_len, ' ')
//                  << "] " << labels[i] << ": " << bins[i] << "\n";
//    }
//}
//
//// --- globals ---
//static int nextId = 1;
//
//// If your types are aliases already, these lines are harmless.
//// Otherwise adjust.
//using IdT  = OrderId;
//using PxT  = Price;
//using QtyT = Quantity;
//
//static double clamp_price(double v) {
//    if (v < 100.0) v = 100.0;
//    if (v > 110.0) v = 110.0;
//    return v;
//}
//
//// Your constructor is: Order(id, type, side, price, quantity)
//static Order make_order(IdT id, double px, int qty, bool is_buy, OrderType type = OrderType::GoodTillCancel) {
//    return Order(
//            id,
//            type,
//            is_buy ? Side::Buy : Side::Sell,
//            static_cast<PxT>(px),
//            static_cast<QtyT>(qty)
//    );
//}
//
//// Your constructor is: OrderModify(id, side, price, quantity)
//static OrderModify make_modify(IdT id, Side side, double new_px, int new_qty) {
//    return OrderModify(
//            id,
//            side,
//            static_cast<PxT>(new_px),
//            static_cast<QtyT>(new_qty)
//    );
//}
//
//// Keep book at target size (same “drifting” idea as your other benchmark)
//static void populate_book(Orderbook& book, int target, std::mt19937& rng, std::vector<int>* track_ids = nullptr) {
//    double buy_curr = 102.5;
//    double ask_curr = 107.5;
//    const double mu = 0.0;
//    const double sigma = 0.008;
//    std::normal_distribution<double> z(0.0, 1.0);
//
//    while ((int)book.size() < target) {
//        bool is_buy = (nextId % 2 == 0);
//        double price;
//
//        if (is_buy) {
//            double step = (mu - 0.5 * sigma * sigma) + sigma * z(rng);
//            buy_curr = buy_curr * std::exp(step);
//            if (buy_curr < 100.0) buy_curr = 100.0;
//            if (buy_curr >= 105.0) buy_curr = 104.999999;
//            price = buy_curr;
//        } else {
//            double step = (mu - 0.5 * sigma * sigma) + sigma * z(rng);
//            ask_curr = ask_curr * std::exp(step);
//            if (ask_curr > 110.0) ask_curr = 110.0;
//            if (ask_curr < 105.0) ask_curr = 105.0;
//            price = ask_curr;
//        }
//
//        // GTC (GoodTillCancel) for background liquidity
//        (void)book.addOrder(make_order(nextId, price, 20, is_buy, OrderType::GoodTillCancel));
//
//        if (track_ids) track_ids->push_back(nextId);
//        ++nextId;
//    }
//}
//
//int main() {
//    std::mt19937 rng(1234);
//    std::uniform_int_distribution<int> op_picker(0, 2);
//
//    std::uniform_real_distribution<double> uniform_price(100.0, 110.0);
//    std::normal_distribution<double> normal_price(105.0, 2.0);
//    std::exponential_distribution<double> exp_price(1.0 / 3.0);
//    std::student_t_distribution<double> t_price(3.0);
//    std::lognormal_distribution<double> logn_price(std::log(105.0), 0.08);
//
//    auto sample_price = [&](int dist_idx) {
//        double v = 100.0;
//        switch (dist_idx) {
//            case 0: v = uniform_price(rng); break;
//            case 1: v = normal_price(rng); break;
//            case 2: v = 100.0 + std::min(exp_price(rng), 10.0); break;
//            case 3: v = 105.0 + t_price(rng) * 1.5; break;
//            case 4: v = logn_price(rng); break;
//            default: v = uniform_price(rng); break;
//        }
//        return clamp_price(v);
//    };
//
//    const char* dist_names[] = {"uniform", "normal", "exponential", "student_t", "lognormal", "uniform"};
//
//    for (int distribution = 0; distribution < 6; ++distribution) {
//        // Disable prune thread for benchmark determinism + less noise
//        Orderbook book(/*startPruneThread=*/false);
//
//        nextId = 1;
//
//        // Track active order ids so cancel/modify uses valid ids
//        std::vector<int> active_ids;
//        active_ids.reserve(RANGE_LOW + BENCH_RUNS);
//
//        populate_book(book, RANGE_LOW, rng, &active_ids);
//        int beginSize = (int)book.size();
//
//        std::vector<unsigned long long> latencies;
//        latencies.reserve(BENCH_RUNS);
//
//        for (int i = 0; i < BENCH_RUNS; ++i) {
//            int op = op_picker(rng);
//
//            unsigned long long t0 = 0, t1 = 0;
//
//            if (op == 0 || active_ids.size() < 5) {
//                // ADD
//                int id = nextId++;
//                bool is_buy = (id & 1) == 0;
//                double px = sample_price(distribution);
//                int qty = 10 + (id % 10);
//
//                t0 = std::chrono::steady_clock::now().time_since_epoch().count();
//                (void)book.addOrder(make_order(id, px, qty, is_buy, OrderType::GoodTillCancel));
//                t1 = std::chrono::steady_clock::now().time_since_epoch().count();
//
//                active_ids.push_back(id);
//            } else if (op == 1) {
//                // CANCEL
//                std::uniform_int_distribution<int> idx_picker(0, (int)active_ids.size() - 1);
//                int idx = idx_picker(rng);
//                int cancel_id = active_ids[idx];
//
//                t0 = std::chrono::steady_clock::now().time_since_epoch().count();
//                book.cancelOrder(cancel_id);
//                t1 = std::chrono::steady_clock::now().time_since_epoch().count();
//
//                std::swap(active_ids[idx], active_ids.back());
//                active_ids.pop_back();
//            } else {
//                // MODIFY (your impl cancels+adds internally; needs side)
//                std::uniform_int_distribution<int> idx_picker(0, (int)active_ids.size() - 1);
//                int idx = idx_picker(rng);
//                int mod_id = active_ids[idx];
//
//                // We don’t store side per id, but we can derive it the same way we created it.
//                // If you ever change that rule, store sides in a parallel vector.
//                Side side = ((mod_id & 1) == 0) ? Side::Buy : Side::Sell;
//
//                double new_px = sample_price(distribution);
//                int new_qty = 10 + (mod_id % 10);
//
//                t0 = std::chrono::steady_clock::now().time_since_epoch().count();
//                (void)book.modifyOrder(make_modify(mod_id, side, new_px, new_qty));
//                t1 = std::chrono::steady_clock::now().time_since_epoch().count();
//            }
//
//            // refill back to RANGE_LOW
//            populate_book(book, RANGE_LOW, rng, nullptr);
//
//            latencies.push_back((t1 > t0) ? (t1 - t0) : 0);
//        }
//
//        std::cout << "=== Distribution " << distribution
//                  << " (" << dist_names[distribution] << ") ===\n";
//        print_stats(latencies);
//        print_histogram(latencies);
//        std::cout << "begin book size: " << beginSize << "\n";
//        std::cout << "end book size: " << book.size() << "\n\n";
//    }
//
//    return 0;
//}
