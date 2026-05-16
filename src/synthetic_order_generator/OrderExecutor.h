#pragma once

#include "orderbook/Orderbook.h"
#include "OrderGenerator.h"

#include <cstddef>
#include <string>
#include <vector>

class OrderExecutor {
public:
    OrderExecutor(const MarketState &state, size_t ticks, std::string persist_path = "");

    double run(const std::string &csv_path = "");

    [[nodiscard]] const std::unique_ptr<Orderbook> &getOrderbook() const;

private:
    std::unique_ptr<Orderbook> orderbook_ = std::make_unique<Orderbook>(true);
    OrderGenerator generator_;
    std::string persist_path_;

    static std::vector<OrderEvent> getOrdersFromCsv(const std::string &path);

    double runFromSimulation();

    [[nodiscard]] double executeOrders(const std::vector<OrderEvent> &events) const;

    [[nodiscard]] double executeOrdersPersist(const std::vector<OrderEvent> &events) const;

    [[nodiscard]] double runFromCsv(const std::string &csv_path) const;

};

