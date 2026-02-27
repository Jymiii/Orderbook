#ifndef ORDERBOOK_ORDEREXECUTOR_H
#define ORDERBOOK_ORDEREXECUTOR_H

#include "orderbook/Orderbook.h"
#include "OrderGenerator.h"
#include "shared/Timer.h"

#include <cstddef>
#include <string>
#include <vector>

class OrderExecutor {
public:
    OrderExecutor() = default;

    OrderExecutor(MarketState state, size_t ticks, std::string persist_path = "");

    double run(const std::string &csv_path = "");

    const std::unique_ptr<Orderbook>& getOrderbook();

private:
    std::unique_ptr<Orderbook> orderbook_ = std::make_unique<Orderbook>(true);
    OrderGenerator generator_{MarketState{}, 100000};
    std::string persist_path_{};

    static std::vector<OrderEvent> getOrdersFromCsv(const std::string &path);

    double runFromSimulation();

    [[nodiscard]] double executeOrders(const std::vector<OrderEvent> &events) const;

    [[nodiscard]] double executeOrdersPersist(const std::vector<OrderEvent> &events) const;

    [[nodiscard]] double runFromCsv(const std::string &csv_path) const;

};

#endif // ORDERBOOK_ORDEREXECUTOR_H
