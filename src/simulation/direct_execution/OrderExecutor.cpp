#include "OrderExecutor.h"

#include "commands/commands.h"
#include "orderbook/Order.h"
#include "orderbook/OrderModify.h"
#include "utils/Timer.h"

#include <fstream>
#include <iostream>
#include <stdexcept>
#include <utility>

OrderExecutor::OrderExecutor(const MarketState &state, const size_t ticks, std::string persist_path)
    : generator_{state, ticks}, persist_path_{std::move(persist_path)} {
}

double OrderExecutor::run(const std::string &csv_path) {
    if (csv_path.empty()) return runFromSimulation();
    return runFromCsv(csv_path);
}

const std::unique_ptr<Orderbook> &OrderExecutor::getOrderbook() const {
    return orderbook_;
}

double OrderExecutor::executeCommands(const std::vector<Command>& commands) const
{
    const Timer timer;

    for (const auto& cmd : commands)
    {
        std::visit(Overloaded{
                       [&](const NewOrderCmd& c)
                       {
                           orderbook_->addOrder(Order{c.id, c.type, c.side, c.price, c.quantity});
                       },
                       [&](const CancelOrderCmd& c) { orderbook_->cancelOrder(c.id); },
                       [&](const ModifyOrderCmd& c)
                       {
                           orderbook_->modifyOrder(OrderModify{c.id, c.side, c.price, c.quantity});
                       },
                       [&](const PruneGFDCmd&) { orderbook_->pruneStaleGoodForDay(); }

                   }, cmd);
    }

    return timer.elapsed();
}

double OrderExecutor::executeCommandsPersist(const std::vector<Command> &commands) const
{
    const Timer timer;
    std::ofstream file(persist_path_, std::ios::out | std::ios::trunc);
    if (!file.is_open())
        throw std::runtime_error("Could not open persist file: " + persist_path_);

    for (const auto& cmd : commands)
    {
        std::visit(Overloaded{
                       [&](const NewOrderCmd& c)
                       {
                           orderbook_->addOrder(Order{c.id, c.type, c.side, c.price, c.quantity});
                           file << 0
                               << ',' << c.id
                               << ',' << +std::to_underlying(c.type)
                               << ',' << +std::to_underlying(c.side)
                               << ',' << c.price
                               << ',' << c.quantity << '\n';
                       },
                       [&](const CancelOrderCmd& c)
                       {
                           orderbook_->cancelOrder(c.id);
                           file << 1 << ',' << c.id << '\n';
                       },
                       [&](const ModifyOrderCmd& c)
                       {
                           orderbook_->modifyOrder(OrderModify{c.id, c.side, c.price, c.quantity});
                           file << 2
                               << ',' << c.id
                               << ',' << +std::to_underlying(c.side)
                               << ',' << c.price
                               << ',' << c.quantity << '\n';
                       }
            ,
                       [&](const PruneGFDCmd&) { orderbook_->pruneStaleGoodForDay(); }
                   }, cmd);
    }

    file.flush();
    return timer.elapsed();
}

double OrderExecutor::runFromSimulation()
{
    const std::vector<Command> commands = generator_.generate();
    if (persist_path_.empty()) return executeCommands(commands);
    return executeCommandsPersist(commands);
}

double OrderExecutor::runFromCsv(const std::string &csv_path) const {
    return executeCommands(getCommandsFromCsv(csv_path));
}

std::vector<Command> OrderExecutor::getCommandsFromCsv(const std::string &path) {
    std::vector<Command> commands;
    std::ifstream file(path);
    std::string str;

    while (std::getline(file, str))
    {
        std::vector<std::string> members;
        auto from = 0uz;
        auto next = str.find(',', from);
        while (next != std::string::npos)
        {
            members.push_back(str.substr(from, next - from));
            from = next + 1;
            next = str.find(',', from);
        }
        members.push_back(str.substr(from));

        if (const int action = std::stoi(members[0]); action == 0)
        {
            commands.emplace_back(NewOrderCmd{
                static_cast<OrderType>(std::stoi(members[2])),
                static_cast<Side>(std::stoi(members[3])),
                static_cast<Price>(std::stoll(members[4])),
                static_cast<OrderId>(std::stoll(members[1])),
                static_cast<Quantity>(std::stoll(members[5]))
            });
        }
        else if (action == 1)
        {
            commands.emplace_back(CancelOrderCmd{
                static_cast<OrderId>(std::stoll(members[1]))
            });
        } else if (action == 2) {
            commands.emplace_back(ModifyOrderCmd{
                static_cast<Side>(std::stoi(members[2])),
                static_cast<Price>(std::stoll(members[3])),
                static_cast<OrderId>(std::stoll(members[1])),
                static_cast<Quantity>(std::stoll(members[4]))
            });
        }
    }
    return commands;
}
