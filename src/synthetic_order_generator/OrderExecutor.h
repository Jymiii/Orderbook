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

    double run(const std::string& csv_path = "");

    Orderbook &getOrderbook();

private:
    Orderbook orderbook_{true};
    OrderGenerator generator_{MarketState{}, 100000};
    std::string persist_path_{};

    double runFromCsv(const std::string& csv_path);

    static std::vector<OrderEvent> getOrdersFromCsv(const std::string& path);

    double executeOrders(std::vector<OrderEvent> &orders);

    double executeOrdersPersist(std::vector<OrderEvent> &orders);

    double runFromSimulation();
};

#endif // ORDERBOOK_ORDEREXECUTOR_H
