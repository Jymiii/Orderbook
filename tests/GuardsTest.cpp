#include "TestHelpers.h"
#include "gtest/gtest.h"

TEST(Guards, ZeroQuantityOrder_IsIgnored) {
    OrderFactory f;
    Orderbook ob{};

    ob.addOrder(f.make(0, OrderType::GoodTillCancel, Side::Sell, 100, 10));

    Order zeroQty{1, OrderType::GoodTillCancel, Side::Buy, 100, 0};
    ob.addOrder(zeroQty);

    EXPECT_TRUE(ob.getTrades().empty());
    EXPECT_EQ(1, ob.size());
    auto info = ob.getOrderInfos();
    EXPECT_EQ(1, info.getAsks().size());
    EXPECT_TRUE(info.getBids().empty());
}

TEST(Guards, DuplicateOrderId_SecondIsIgnored) {
    OrderFactory f;
    Orderbook ob{};

    ob.addOrder(f.make(42, OrderType::GoodTillCancel, Side::Buy, 100, 5));
    EXPECT_TRUE(ob.getTrades().empty());
    EXPECT_EQ(1, ob.size());

    ob.addOrder(f.make(42, OrderType::GoodTillCancel, Side::Buy, 100, 5));
    EXPECT_TRUE(ob.getTrades().empty());
    EXPECT_EQ(1, ob.size());

    auto info = ob.getOrderInfos();
    ASSERT_EQ(1, info.getBids().size());
    EXPECT_EQ(100, info.getBids()[0].price);
    EXPECT_EQ(5, info.getBids()[0].quantity);
}

TEST(Guards, CancelNonExistentId_IsNoOp) {
    OrderFactory f;
    Orderbook ob{};

    ob.addOrder(f.make(0, OrderType::GoodTillCancel, Side::Buy, 100, 10));
    ob.addOrder(f.make(1, OrderType::GoodTillCancel, Side::Sell, 110, 5));

    EXPECT_NO_FATAL_FAILURE(ob.cancelOrder(999));

    EXPECT_EQ(2, ob.size());
    auto info = ob.getOrderInfos();
    ASSERT_EQ(1, info.getBids().size());
    ASSERT_EQ(1, info.getAsks().size());
}

TEST(Guards, CancelAlreadyCancelledId_IsNoOp) {
    OrderFactory f;
    Orderbook ob{};

    ob.addOrder(f.make(0, OrderType::GoodTillCancel, Side::Buy, 100, 10));
    ob.cancelOrder(0);
    EXPECT_EQ(0, ob.size());

    EXPECT_NO_FATAL_FAILURE(ob.cancelOrder(0));
    EXPECT_EQ(0, ob.size());
}
