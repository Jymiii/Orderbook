#ifndef ORDERBOOK_ORDEREXECUTOR_H
#define ORDERBOOK_ORDEREXECUTOR_H

#include "orderbook/Orderbook.h"
#include "OrderGenerator.h"
#include "Timer.h"

#include <cstddef>
#include <string>
#include <vector>

class OrderExecutor {
public:
    OrderExecutor() = default;

    OrderExecutor(MarketState state, size_t ticks, std::string persist_path = "");

    double run(std::string csv_path = "");

    Orderbook &getOrderbook();

private:
    Orderbook orderbook_{false};
    OrderGenerator generator_{MarketState{}, 100000};
    std::string persist_path_{};

    double runFromCsv(std::string csv_path);

    static std::vector<Order> getOrdersFromCsv(std::string path);

    double executeOrders(std::vector<Order> &orders);

    double executeOrdersPersist(std::vector<Order> &orders);

    double runFromSimulation();
};

#endif // ORDERBOOK_ORDEREXECUTOR_H
