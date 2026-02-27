//
// Created by Jimi van der Meer on 13/02/2026.
//
#include "OrderExecutor.h"

#include <fstream>
#include <iostream>
#include <utility>

OrderExecutor::OrderExecutor(MarketState state, size_t ticks, std::string persist_path)
    : generator_{state, ticks}, persist_path_{std::move(persist_path)} {
}

double OrderExecutor::run(const std::string &csv_path) {
    if (csv_path.empty()) {
        return runFromSimulation();
    } else {
        return runFromCsv(csv_path);
    }
}

const std::unique_ptr<Orderbook>& OrderExecutor::getOrderbook() {
    return orderbook_;
}

double OrderExecutor::executeOrders(const std::vector<OrderEvent> &events) const {
    const Timer timer;

    for (const auto &e: events) {
        std::visit(Overloaded{
                       [&](Order const &o) {
                           orderbook_->addOrder(o);
                       },
                       [&](OrderModify const &m) {
                           orderbook_->modifyOrder(m);
                       },
                       [&](OrderId const &id) {
                           orderbook_->cancelOrder(id);
                       }
                   }, e.payload);
    }

    return timer.elapsed();
}


double OrderExecutor::executeOrdersPersist(const std::vector<OrderEvent> &events) const {
    const Timer timer;
    std::ofstream file(persist_path_, std::ios::out | std::ios::trunc);
    if (!file.is_open()) {
        throw std::runtime_error("Could not open persist file: " + persist_path_);
    }
    for (const auto &e: events) {
        std::visit(Overloaded{
                       [&](Order const &o) {
                           orderbook_->addOrder(o);
                       },
                       [&](OrderModify const &m) {
                           orderbook_->modifyOrder(m);
                       },
                       [&](OrderId const &id) {
                           orderbook_->cancelOrder(id);
                       }
                   }, e.payload);

        file << e;
    }
    file.flush();
    const auto duration = timer.elapsed();
    return duration;
}

double OrderExecutor::runFromSimulation() {
    const std::vector<OrderEvent> events = generator_.generate();
    if (persist_path_.empty()) return executeOrders(events);
    else return executeOrdersPersist(events);
}

double OrderExecutor::runFromCsv(const std::string &csv_path) const {
    const std::vector<OrderEvent> orders = getOrdersFromCsv(csv_path);
    return executeOrders(orders);
}

std::vector<OrderEvent> OrderExecutor::getOrdersFromCsv(const std::string &path) {
    std::vector<OrderEvent> events;
    std::ifstream file(path);
    std::string str;
    while (std::getline(file, str)) {
        std::vector<std::string> members;
        auto from = 0uz;
        auto next = str.find(',', from);
        while (next != std::string::npos) {
            members.push_back(str.substr(from, (next - from)));
            from = next + 1;
            next = str.find(',', from);
        }
        members.push_back(str.substr(from, next));

        int action = std::stoi(members[0]);

        if (action == static_cast<int>(EventType::New)) {
            events.emplace_back(EventType::New,
                                Order{
                                    static_cast<OrderId>(std::stoll(members[1])),
                                    static_cast<OrderType>(std::stoi(members[2])),
                                    static_cast<Side>(std::stoi(members[3])),
                                    static_cast<Price>(std::stoll(members[4])),
                                    static_cast<Quantity>(std::stoll(members[5]))
                                });
        } else if (action == static_cast<int>(EventType::Modify)) {
            events.emplace_back(EventType::Modify,
                                OrderModify{
                                    static_cast<OrderId>(std::stoll(members[1])),
                                    static_cast<Side>(std::stoi(members[2])),
                                    static_cast<Price>(std::stoll(members[3])),
                                    static_cast<Quantity>(std::stoll(members[4]))
                                });
        } else if (action == static_cast<int>(EventType::Cancel)) {
            events.push_back(OrderEvent::Cancel(static_cast<OrderId>(std::stoll(members[1]))));
        }
    }
    return events;
}