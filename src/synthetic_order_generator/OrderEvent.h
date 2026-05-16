//
// Created by Jimi van der Meer on 12/02/2026.
//
#ifndef ORDERBOOK_ORDEREVENT_H
#define ORDERBOOK_ORDEREVENT_H

#include <variant>
#include <type_traits>
#include <ostream>
#include "orderbook/Order.h"
#include "orderbook/OrderModify.h"


template<class... Ts>
struct Overloaded : Ts... {
    using Ts::operator()...;
};

template<class... Ts>
Overloaded(Ts...) -> Overloaded<Ts...>;

enum class EventType : int {
    New = 0, Cancel = 1, Modify = 2
};

struct OrderEvent {
    using Payload = std::variant<Order, OrderModify, OrderId>;

    EventType event_{};
    Payload payload;

    // Convenience constructors (so emplace_back is clean)
    static OrderEvent New(Order o) { return {EventType::New, std::move(o)}; }

    static OrderEvent Modify(OrderModify m) { return {EventType::Modify, std::move(m)}; }

    static OrderEvent Cancel(OrderId id) { return {EventType::Cancel, id}; }

    OrderEvent(EventType t, Payload p) : event_(t), payload(std::move(p)) {
    }

    friend std::ostream &operator<<(std::ostream &os, const OrderEvent &e) {
        std::visit(Overloaded{
                       [&](Order const &o) {
                           os << std::to_underlying(EventType::New)
                                   << "," << o.getId()
                                   << "," << std::to_underlying(o.getType())
                                   << "," << std::to_underlying(o.getSide())
                                   << "," << o.getPrice()
                                   << "," << o.getRemainingQuantity()
                                   << "\n";
                       },
                       [&](OrderModify const &m) {
                           os << std::to_underlying(EventType::Modify)
                                   << "," << m.getId()
                                   << "," << std::to_underlying(m.getSide())
                                   << "," << m.getPrice()
                                   << "," << m.getQuantity()
                                   << "\n";
                       },
                       [&](OrderId const &id) {
                           os << std::to_underlying(EventType::Cancel)
                                   << "," << id
                                   << "\n";
                       }
                   }, e.payload);

        return os;
    }
};

#endif //ORDERBOOK_ORDEREVENT_H