#pragma once

#include <variant>
#include "orderbook/OrderType.h"
#include "orderbook/Side.h"
#include "orderbook/Types.h"

template <class... Ts>
struct Overloaded : Ts...
{
    using Ts::operator()...;
};

template <class... Ts>
Overloaded(Ts...) -> Overloaded<Ts...>;

struct NewOrderCmd
{
    OrderType type;
    Side side;
    Price price;
    OrderId id;
    Quantity quantity;
};

struct CancelOrderCmd
{
    OrderId id;
};

struct ModifyOrderCmd
{
    Side side;
    Price price;
    OrderId id;
    Quantity quantity;
};

using Command = std::variant<NewOrderCmd, CancelOrderCmd, ModifyOrderCmd>;
