//
// Created by Jimi van der Meer on 13/02/2026.
//
#include "OrderExecutor.h"

#include <fstream>
#include <iostream>

OrderExecutor::OrderExecutor(MarketState state, size_t ticks, std::string persist_path)
        : generator_{state, ticks}, persist_path_{ persist_path } {}

void OrderExecutor::run(std::string csv_path) {
    if (csv_path.empty()) {
        runFromSimulation();
    } else {
        runFromCsv(csv_path);
    }
}

Orderbook &OrderExecutor::getOrderbook() {
    return orderbook_;
}

void OrderExecutor::executeOrders(std::vector<Order> &orders) {
    Timer timer;
    for (const auto &order: orders) {
        orderbook_.addOrder(order);
    }
    auto duration = timer.elapsed();
    std::cout << "Processing " << orders.size() << " orders took " << duration << " seconds.";
}

void OrderExecutor::executeOrdersPersist(std::vector<Order> &orders) {
    Timer timer;
    std::ofstream file { persist_path_ };
    for (const auto &order: orders) {
        orderbook_.addOrder(order);
        file << order;
    }
    auto duration = timer.elapsed();
    std::cout << "Processing " << orders.size() << " orders took " << duration << " seconds.";
}

void OrderExecutor::runFromSimulation() {
    std::vector<Order> orders = generator_.generate();
    if (persist_path_.empty()) executeOrders(orders);
    else executeOrdersPersist(orders);
}

void OrderExecutor::runFromCsv(std::string csv_path) {
    std::vector<Order> orders = getOrdersFromCsv(csv_path);
    executeOrders(orders);
}

std::vector<Order> OrderExecutor::getOrdersFromCsv(std::string path) {
    std::vector<Order> orders;
    std::ifstream file(path);
    std::string str;
    while (std::getline(file, str)) {
        std::vector<std::string> members;
        auto from = 0;
        auto next = str.find(',', from);
        while (next != std::string::npos) {
            members.push_back(str.substr(from, (next - from)));
            from = next + 1;
            next = str.find(',', from);
        }
        members.push_back(str.substr(from, next));

        orders.emplace_back(
                static_cast<OrderId>(std::stoll(members[0])),
                static_cast<OrderType>(std::stoi(members[1])),
                static_cast<Side>(std::stoi(members[2])),
                static_cast<Price>(std::stoll(members[3])),
                static_cast<Quantity>(std::stoll(members[4]))
        );
    }
    return orders;
}

