//
// Created by Jimi van der Meer on 11/02/2026.
//
#include <gtest/gtest.h>
#include "OrderModify.h"

TEST(OrderModify, StoresFieldsAndConvertsToOrderPtr) {
constexpr OrderId id = 42;
constexpr Side side = Side::Buy;
constexpr Price price = 123;
constexpr Quantity qty = 7;

OrderModify mod{id, side, price, qty};

EXPECT_EQ(mod.getId(), id);
EXPECT_EQ(mod.getSide(), side);
EXPECT_EQ(mod.getPrice(), price);
EXPECT_EQ(mod.getQuantity(), qty);

auto order = mod.toOrderPtr(OrderType::GoodTillCancel);
ASSERT_NE(order, nullptr);

EXPECT_EQ(order->getId(), id);
EXPECT_EQ(order->getSide(), side);
EXPECT_EQ(order->getPrice(), price);
EXPECT_EQ(order->getRemainingQuantity(), qty);
EXPECT_EQ(order->getType(), OrderType::GoodTillCancel);
}
