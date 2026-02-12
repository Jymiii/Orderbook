//
// Created by Jimi van der Meer on 12/02/2026.
//

#include "TestHelpers.h"

TEST(FillOrKill, NoCounterParty) {
    OrderFactory f;
    Orderbook ob{};

    Trades trades_one = ob.addOrder(f.make(OrderType::FillOrKill, Side::Buy, 50, 1));
    Trades trades_two = ob.addOrder(f.make(OrderType::FillOrKill, Side::Sell, 50, 1));
    EXPECT_EQ(ob.size(), 0);
    auto info = ob.getOrderInfos();
    EXPECT_EQ(info.getBids().size(), 0);
    EXPECT_EQ(info.getAsks().size(), 0);
    EXPECT_TRUE((trades_one.empty()));
    EXPECT_TRUE((trades_two.empty()));
}

TEST(FillOrKill, CantFullyFillSimple) {
    OrderFactory f;
    Orderbook ob{};

    Trades trades_one = ob.addOrder(f.make(OrderType::GoodTillCancel, Side::Buy, 50, 10));
    Trades trades_two = ob.addOrder(f.make(OrderType::FillOrKill, Side::Sell, 50, 11));
    EXPECT_EQ(ob.size(), 1);
    auto info = ob.getOrderInfos();
    EXPECT_EQ(info.getBids().size(), 1);
    EXPECT_EQ(info.getAsks().size(), 0);
    EXPECT_TRUE((trades_one.empty()));
    EXPECT_TRUE((trades_two.empty()));
}

TEST(FillOrKill, CanFullyFillSimple) {
    OrderFactory f;
    Orderbook ob{};

    Trades trades_one = ob.addOrder(f.make(0, OrderType::GoodTillCancel, Side::Buy, 50, 10));
    Trades trades_two = ob.addOrder(f.make(1, OrderType::FillOrKill, Side::Sell, 50, 10));
    EXPECT_EQ(ob.size(), 0);
    auto info = ob.getOrderInfos();
    EXPECT_EQ(info.getBids().size(), 0);
    EXPECT_EQ(info.getAsks().size(), 0);
    EXPECT_TRUE((trades_one.empty()));
    EXPECT_EQ(1, trades_two.size());
    EXPECT_TRUE(hasTradeLike(trades_two, {0, 1, 50, 50, 10}));
}

TEST(FillOrKill, CantFullyFillBig) {
    OrderFactory f;
    Orderbook ob{};

    ob.addOrder(f.make(0, OrderType::GoodTillCancel, Side::Buy, 50, 10));
    ob.addOrder(f.make(1, OrderType::GoodTillCancel, Side::Buy, 50, 10));
    ob.addOrder(f.make(2, OrderType::GoodTillCancel, Side::Buy, 52, 15));
    ob.addOrder(f.make(3, OrderType::GoodTillCancel, Side::Buy, 53, 4));
    ob.addOrder(f.make(4, OrderType::GoodTillCancel, Side::Buy, 60, 10));

    Trades trades = ob.addOrder(f.make(OrderType::FillOrKill, Side::Sell, 50, 50));
    EXPECT_EQ(ob.size(), 5);
    auto info = ob.getOrderInfos();
    EXPECT_EQ(info.getBids().size(), 4);
    EXPECT_EQ(info.getAsks().size(), 0);
    EXPECT_TRUE((trades.empty()));

    trades = ob.addOrder(f.make(OrderType::FillOrKill, Side::Sell, 51, 30));
    EXPECT_EQ(ob.size(), 5);
    info = ob.getOrderInfos();
    EXPECT_EQ(info.getBids().size(), 4);
    EXPECT_EQ(info.getAsks().size(), 0);
    EXPECT_TRUE((trades.empty()));

    trades = ob.addOrder(f.make(OrderType::FillOrKill, Side::Sell, 52, 30));
    EXPECT_EQ(ob.size(), 5);
    info = ob.getOrderInfos();
    EXPECT_EQ(info.getBids().size(), 4);
    EXPECT_EQ(info.getAsks().size(), 0);
    EXPECT_TRUE((trades.empty()));

    trades = ob.addOrder(f.make(OrderType::FillOrKill, Side::Sell, 61, 1));
    EXPECT_EQ(ob.size(), 5);
    info = ob.getOrderInfos();
    EXPECT_EQ(info.getBids().size(), 4);
    EXPECT_EQ(info.getAsks().size(), 0);
    EXPECT_TRUE((trades.empty()));
}

TEST(FillOrKill, CanFullyFillBig) {
    OrderFactory f;
    Orderbook ob{};

    ob.addOrder(f.make(0, OrderType::GoodTillCancel, Side::Sell, 50, 10));
    ob.addOrder(f.make(1, OrderType::GoodTillCancel, Side::Sell, 50, 10));
    ob.addOrder(f.make(2, OrderType::GoodTillCancel, Side::Sell, 52, 15));
    ob.addOrder(f.make(3, OrderType::GoodTillCancel, Side::Sell, 53, 4));
    ob.addOrder(f.make(4, OrderType::GoodTillCancel, Side::Sell, 55, 10));

    Trades trades = ob.addOrder(f.make(5, OrderType::FillOrKill, Side::Buy, 55, 40));
    EXPECT_EQ(ob.size(), 1);
    auto info = ob.getOrderInfos();
    EXPECT_EQ(info.getBids().size(), 0);
    EXPECT_EQ(info.getAsks().size(), 1);
    EXPECT_EQ(5, trades.size());
    EXPECT_TRUE(hasTradeLike(trades, {5, 0, 55, 50, 10}));
    EXPECT_TRUE(hasTradeLike(trades, {5, 1, 55, 50, 10}));
    EXPECT_TRUE(hasTradeLike(trades, {5, 2, 55, 52, 15}));
    EXPECT_TRUE(hasTradeLike(trades, {5, 3, 55, 53, 4}));
    EXPECT_TRUE(hasTradeLike(trades, {5, 4, 55, 55, 1}));
}

TEST(FillOrKill, CanFullyFillBigBuy) {
    OrderFactory f;
    Orderbook ob{};

    ob.addOrder(f.make(0, OrderType::GoodTillCancel, Side::Buy, 50, 10));
    ob.addOrder(f.make(1, OrderType::GoodTillCancel, Side::Buy, 50, 10));
    ob.addOrder(f.make(2, OrderType::GoodTillCancel, Side::Buy, 52, 15));
    ob.addOrder(f.make(3, OrderType::GoodTillCancel, Side::Buy, 53, 4));
    ob.addOrder(f.make(4, OrderType::GoodTillCancel, Side::Buy, 55, 10));

    Trades trades = ob.addOrder(f.make(5, OrderType::FillOrKill, Side::Sell, 50, 40));
    EXPECT_EQ(ob.size(), 1);
    auto info = ob.getOrderInfos();
    EXPECT_EQ(info.getBids().size(), 1);
    EXPECT_EQ(info.getAsks().size(), 0);
    EXPECT_EQ(5, trades.size());
    EXPECT_TRUE(hasTradeLike(trades, {4, 5, 55, 50, 10}));
    EXPECT_TRUE(hasTradeLike(trades, {3, 5, 53, 50, 4}));
    EXPECT_TRUE(hasTradeLike(trades, {2, 5, 52, 50, 15}));
    // TimeOrder test
    EXPECT_TRUE(hasTradeLike(trades, {1, 5, 50, 50, 1}));
    EXPECT_TRUE(hasTradeLike(trades, {0, 5, 50, 50, 10}));
}