#include "TestHelpers.h"
#include "gtest/gtest.h"

TEST(MarketOrder, AddBuys_NoSells) {
    OrderFactory f;
    Orderbook ob{};

    ob.addOrder(f.make(OrderType::Market, Side::Buy, 50, 1));
    ob.addOrder(f.make(OrderType::Market, Side::Buy, 50, 1));
    ob.addOrder(f.make(OrderType::Market, Side::Buy, 60, 10));
    EXPECT_EQ(ob.size(), 0);

    auto info = ob.getOrderInfos();
    EXPECT_EQ(info.getBids().size(), 0);
    EXPECT_EQ(info.getAsks().size(), 0);
}

TEST(MarketOrder, AddSells_NoBuys) {
    OrderFactory f;
    Orderbook ob{};

    ob.addOrder(f.make(OrderType::Market, Side::Sell, 50, 1));
    ob.addOrder(f.make(OrderType::Market, Side::Sell, 50, 1));
    ob.addOrder(f.make(OrderType::Market, Side::Sell, 60, 10));
    EXPECT_EQ(ob.size(), 0);

    auto info = ob.getOrderInfos();
    EXPECT_EQ(info.getBids().size(), 0);
    EXPECT_EQ(info.getAsks().size(), 0);
}

TEST(MarketOrder, SweepSells) {
    OrderFactory f;
    Orderbook ob{};

    ob.addOrder(f.make(0, OrderType::GoodTillCancel, Side::Sell, 100, 10));
    ob.addOrder(f.make(1, OrderType::GoodTillCancel, Side::Sell, 101, 15));
    ob.addOrder(f.make(2, Side::Buy, 10));
    auto info = ob.getOrderInfos();
    EXPECT_EQ(1, ob.size());
    EXPECT_EQ(1, ob.getTrades().size());
    EXPECT_EQ(info.getBids().size(), 0);
    EXPECT_EQ(info.getAsks().size(), 1);
    EXPECT_TRUE(hasTradeLike(ob.getTrades(), {2, 0, 101, 100, 10}));
}

TEST(MarketOrder, SweepBuys) {
    OrderFactory f;
    Orderbook ob{};

    ob.addOrder(f.make(0, OrderType::GoodTillCancel, Side::Buy, 100, 10));
    ob.addOrder(f.make(1, OrderType::GoodTillCancel, Side::Buy, 101, 15));
    ob.addOrder(f.make(2, OrderType::GoodTillCancel, Side::Buy, 102, 30));

    ob.addOrder(f.make(3, Side::Sell, 80));
    auto info = ob.getOrderInfos();
    EXPECT_EQ(0, ob.size());
    EXPECT_EQ(3, ob.getTrades().size());
    EXPECT_EQ(info.getBids().size(), 0);
    EXPECT_EQ(info.getAsks().size(), 0);
}

TEST(MarketOrder, SweepBuysButHasQuantityLimit) {
    OrderFactory f;
    Orderbook ob{};

    ob.addOrder(f.make(0, OrderType::GoodTillCancel, Side::Buy, 100, 10));
    ob.addOrder(f.make(1, OrderType::GoodTillCancel, Side::Buy, 101, 15));
    ob.addOrder(f.make(2, OrderType::GoodTillCancel, Side::Buy, 102, 30));
    ob.addOrder(f.make(3, OrderType::GoodTillCancel, Side::Buy, 102, 30));

    ob.addOrder(f.make(4, Side::Sell, 80));
    auto info = ob.getOrderInfos();
    EXPECT_EQ(1, ob.size());
    EXPECT_EQ(4, ob.getTrades().size());
    EXPECT_EQ(info.getBids().size(), 1);
    EXPECT_EQ(info.getAsks().size(), 0);
    EXPECT_TRUE(hasTradeLike(ob.getTrades(), {0, 4, 100, 100, 5}));
    EXPECT_TRUE(hasTradeLike(ob.getTrades(), {3, 4, 102, 100, 30}));

}
TEST(MarketOrder, SweepsEntireBook) {
    OrderFactory f;
    Orderbook ob{};

    ob.addOrder(f.make(0, OrderType::GoodTillCancel, Side::Sell, 100, 10));
    ob.addOrder(f.make(1, OrderType::GoodTillCancel, Side::Sell, 101, 10));
    ob.addOrder(f.make(2, OrderType::GoodTillCancel, Side::Sell, 102, 10));

    ob.addOrder(f.make(3, Side::Buy, 100));

    EXPECT_EQ(3, ob.getTrades().size());
    EXPECT_TRUE(hasTradeLike(ob.getTrades(), {3, 0, 102, 100, 10}));
    EXPECT_TRUE(hasTradeLike(ob.getTrades(), {3, 1, 102, 101, 10}));
    EXPECT_TRUE(hasTradeLike(ob.getTrades(), {3, 2, 102, 102, 10}));

    EXPECT_EQ(0, ob.size());
    auto info = ob.getOrderInfos();
    EXPECT_TRUE(info.getAsks().empty());
    ASSERT_EQ(0, info.getBids().size());
}

TEST(MarketOrder, TradePrice_IsRestingOrderPrice_NotMarketPrice) {
    OrderFactory f;
    Orderbook ob{};

    ob.addOrder(f.make(0, OrderType::GoodTillCancel, Side::Buy, 105, 5));
    ob.addOrder(f.make(1, OrderType::GoodTillCancel, Side::Buy, 100, 5));

    ob.addOrder(f.make(2, Side::Sell, 8));

    ASSERT_EQ(2, ob.getTrades().size());
    EXPECT_TRUE(hasTradeLike(ob.getTrades(), {0, 2, 105, 100, 5}));
    EXPECT_TRUE(hasTradeLike(ob.getTrades(), {1, 2, 100, 100, 3}));
}

TEST(MarketOrder, AddMarketOrder_WhenCounterBookBecomesEmpty_ResidualsRest) {
    OrderFactory f;
    Orderbook ob{};

    ob.addOrder(f.make(0, OrderType::GoodTillCancel, Side::Sell, 50, 5));
    ob.addOrder(f.make(1, Side::Buy, 10));

    ASSERT_EQ(1, ob.getTrades().size());
    EXPECT_TRUE(hasTradeLike(ob.getTrades(), {1, 0, 50, 50, 5}));

    EXPECT_EQ(0, ob.size());
    auto info = ob.getOrderInfos();
    EXPECT_TRUE(info.getAsks().empty());
    EXPECT_TRUE(info.getBids().empty());
}
