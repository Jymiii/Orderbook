//
// Created by Jimi van der Meer on 12/02/2026.
//
#include "TestHelpers.h"

TEST(FillAndKill, NoCounterParty) {
    OrderFactory f;
    Orderbook ob{};

    ob.addOrder(f.make(OrderType::FillAndKill, Side::Buy, 50, 1));
    ob.addOrder(f.make(OrderType::FillAndKill, Side::Sell, 50, 1));

    EXPECT_EQ(ob.size(), 0);
    auto info = ob.getOrderInfos();
    EXPECT_TRUE(info.getBids().empty());
    EXPECT_TRUE(info.getAsks().empty());
    EXPECT_TRUE(ob.getTrades().empty());
    EXPECT_TRUE(ob.getTrades().empty());
}

TEST(FillAndKill, PartialFill_Simple) {
    OrderFactory f;
    Orderbook ob{};

    ob.addOrder(f.make(0, OrderType::GoodTillCancel, Side::Buy, 50, 10));

    ob.addOrder(f.make(1, OrderType::FillAndKill, Side::Sell, 50, 15));

    EXPECT_EQ(0, ob.size());  // Buy fully matched, remaining sell qty canceled
    auto info = ob.getOrderInfos();
    EXPECT_TRUE(info.getBids().empty());
    EXPECT_TRUE(info.getAsks().empty());

    EXPECT_EQ(1, ob.getTrades().size());
    EXPECT_TRUE(hasTradeLike(ob.getTrades(), {0, 1, 50, 50, 10}));
}

TEST(FillAndKill, PartialFill_Big) {
    OrderFactory f;
    Orderbook ob{};

    ob.addOrder(f.make(0, OrderType::GoodTillCancel, Side::Sell, 50, 10));
    ob.addOrder(f.make(1, OrderType::GoodTillCancel, Side::Sell, 52, 15));
    ob.addOrder(f.make(2, OrderType::GoodTillCancel, Side::Sell, 55, 10));

    ob.addOrder(f.make(3, OrderType::FillAndKill, Side::Buy, 55, 40));

    EXPECT_EQ(0, ob.size());  // all sells consumed, remainder canceled
    auto info = ob.getOrderInfos();
    EXPECT_TRUE(info.getBids().empty());
    EXPECT_TRUE(info.getAsks().empty());

    EXPECT_EQ(3, ob.getTrades().size());
    EXPECT_TRUE(hasTradeLike(ob.getTrades(), {3, 0, 55, 50, 10}));
    EXPECT_TRUE(hasTradeLike(ob.getTrades(), {3, 1, 55, 52, 15}));
    EXPECT_TRUE(hasTradeLike(ob.getTrades(), {3, 2, 55, 55, 10}));
}

TEST(FillAndKill, LeavesBookIfNotFullyMatchedOtherSide) {
    OrderFactory f;
    Orderbook ob{};

    ob.addOrder(f.make(0, OrderType::GoodTillCancel, Side::Sell, 50, 10));
    ob.addOrder(f.make(1, OrderType::GoodTillCancel, Side::Sell, 52, 15));
    ob.addOrder(f.make(2, OrderType::GoodTillCancel, Side::Sell, 55, 10));

    ob.addOrder(f.make(3, OrderType::FillAndKill, Side::Buy, 52, 20));

    EXPECT_EQ(2, ob.size());
    auto info = ob.getOrderInfos();

    EXPECT_TRUE(info.getBids().empty());
    ASSERT_EQ(2, info.getAsks().size());

    EXPECT_EQ(52, info.getAsks()[0].price_);
    EXPECT_EQ(5, info.getAsks()[0].quantity_);

    EXPECT_EQ(55, info.getAsks()[1].price_);
    EXPECT_EQ(10, info.getAsks()[1].quantity_);

    ASSERT_EQ(2, ob.getTrades().size());
    EXPECT_TRUE(hasTradeLike(ob.getTrades(), {3, 0, 52, 50, 10}));
    EXPECT_TRUE(hasTradeLike(ob.getTrades(), {3, 1, 52, 52, 10}));
}


TEST(FillAndKill, OppositeSide_BuyBook) {
    OrderFactory f;
    Orderbook ob{};

    ob.addOrder(f.make(0, OrderType::GoodTillCancel, Side::Buy, 55, 10));
    ob.addOrder(f.make(1, OrderType::GoodTillCancel, Side::Buy, 53, 4));
    ob.addOrder(f.make(2, OrderType::GoodTillCancel, Side::Buy, 52, 15));

    ob.addOrder(f.make(3, OrderType::FillAndKill, Side::Sell, 52, 20));

    EXPECT_EQ(1, ob.size());  // remaining bid level
    auto info = ob.getOrderInfos();
    EXPECT_EQ(1, info.getBids().size());
    EXPECT_EQ(52, info.getBids()[0].price_);
    EXPECT_EQ(9, info.getBids()[0].quantity_);
    EXPECT_TRUE(info.getAsks().empty());

    EXPECT_EQ(3, ob.getTrades().size());
    EXPECT_TRUE(hasTradeLike(ob.getTrades(), {0, 3, 55, 52, 10}));
    EXPECT_TRUE(hasTradeLike(ob.getTrades(), {1, 3, 53, 52, 4}));
    EXPECT_TRUE(hasTradeLike(ob.getTrades(), {2, 3, 52, 52, 6}));
}

TEST(FillAndKill, StopsDueToPrice_NotQuantity) {
    OrderFactory f;
    Orderbook ob{};

    ob.addOrder(f.make(0, OrderType::GoodTillCancel, Side::Sell, 50, 10));
    ob.addOrder(f.make(1, OrderType::GoodTillCancel, Side::Sell, 52, 10));
    ob.addOrder(f.make(2, OrderType::GoodTillCancel, Side::Sell, 60, 10)); // too expensive

    ob.addOrder(f.make(3, OrderType::FillAndKill, Side::Buy, 55, 30));

    EXPECT_EQ(1, ob.size());
    auto info = ob.getOrderInfos();

    EXPECT_TRUE(info.getBids().empty());
    ASSERT_EQ(1, info.getAsks().size());
    EXPECT_EQ(60, info.getAsks()[0].price_);
    EXPECT_EQ(10, info.getAsks()[0].quantity_);

    ASSERT_EQ(2, ob.getTrades().size());
    EXPECT_TRUE(hasTradeLike(ob.getTrades(), {3, 0, 55, 50, 10}));
    EXPECT_TRUE(hasTradeLike(ob.getTrades(), {3, 1, 55, 52, 10}));
}

TEST(FillAndKill, StopsDueToPrice_NotQuantity_BuySide) {
    OrderFactory f;
    Orderbook ob{};

    ob.addOrder(f.make(0, OrderType::GoodTillCancel, Side::Buy, 60, 10));
    ob.addOrder(f.make(1, OrderType::GoodTillCancel, Side::Buy, 58, 10));
    ob.addOrder(f.make(2, OrderType::GoodTillCancel, Side::Buy, 50, 10)); // too cheap

    ob.addOrder(f.make(3, OrderType::FillAndKill, Side::Sell, 55, 30));

    EXPECT_EQ(1, ob.size());
    auto info = ob.getOrderInfos();

    EXPECT_TRUE(info.getAsks().empty());
    ASSERT_EQ(1, info.getBids().size());
    EXPECT_EQ(50, info.getBids()[0].price_);
    EXPECT_EQ(10, info.getBids()[0].quantity_);

    ASSERT_EQ(2, ob.getTrades().size());
    EXPECT_TRUE(hasTradeLike(ob.getTrades(), {0, 3, 60, 55, 10}));
    EXPECT_TRUE(hasTradeLike(ob.getTrades(), {1, 3, 58, 55, 10}));
}

