#pragma once

#include "orderbook/Orderbook.h"
#include "simulation/generation/OrderGenerator.h"

#include <cstddef>
#include <string>
#include <vector>

class OrderExecutor {
public:
    OrderExecutor(const MarketState &state, size_t ticks, std::string persist_path = "");

    double run(const std::string &csv_path = "");

    [[nodiscard]] const std::unique_ptr<Orderbook> &getOrderbook() const;

private:
    std::unique_ptr<Orderbook> orderbook_ = std::make_unique<Orderbook>();
    OrderGenerator generator_;
    std::string persist_path_;

    static std::vector<Command> getCommandsFromCsv(const std::string& path);

    double runFromSimulation();

    [[nodiscard]] double executeCommands(const std::vector<Command>& commands) const;
    [[nodiscard]] double executeCommandsPersist(const std::vector<Command>& commands) const;
    [[nodiscard]] double runFromCsv(const std::string &csv_path) const;
};
