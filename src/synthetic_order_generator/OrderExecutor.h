#ifndef ORDERBOOK_ORDEREXECUTOR_H
#define ORDERBOOK_ORDEREXECUTOR_H

#include "orderbook/Orderbook.h"
#include "OrderGenerator.h"
#include <cstddef>
#include <string>
#include "Timer.h"
#include <iostream>

class OrderExecutor {
public:
    OrderExecutor() = default;

    OrderExecutor(MarketState state, size_t ticks)
            : generator_{state, ticks} {}

    explicit OrderExecutor(std::string csv_path)
            : csv_path_{std::move(csv_path)} {}

    void run() {
        if (csv_path_.empty()){
            runFromSimulation();
        }
        else{
            runFromCsv();
        }
    }

    Orderbook& getOrderbook(){
        return orderbook_;
    }

private:
    Orderbook orderbook_{false};
    OrderGenerator generator_{MarketState{}, 100000};
    std::string csv_path_{};

    void runFromCsv() {}
    void runFromSimulation() {
        std::vector<Order> orders = generator_.generate();
        Timer timer;
        for (const auto& order : orders) {
            orderbook_.addOrder(order);
        }
        std::cout << "Processing " << orders.size() << " orders took " << timer.elapsed() << " seconds.";
    }
};

#endif // ORDERBOOK_ORDEREXECUTOR_H
