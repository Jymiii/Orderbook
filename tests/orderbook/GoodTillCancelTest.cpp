#include <algorithm>
#include "TestHelpers.h"

TEST(GoodTillCancel, Empty) {
    Orderbook ob{};
    auto info = ob.getOrderInfos();

    EXPECT_EQ(ob.size(), 0);
    EXPECT_EQ(info.getBids().size(), 0);
    EXPECT_EQ(info.getAsks().size(), 0);
}

TEST(GoodTillCancel, AddBuys_AggregatesLevels) {
    OrderFactory f;
    Orderbook ob{};

    ob.addOrder(f.make(OrderType::GoodTillCancel, Side::Buy, 50, 1));
    ob.addOrder(f.make(OrderType::GoodTillCancel, Side::Buy, 50, 1));
    ob.addOrder(f.make(OrderType::GoodTillCancel, Side::Buy, 60, 10));
    EXPECT_EQ(ob.size(), 3);

    auto info = ob.getOrderInfos();
    EXPECT_EQ(info.getBids().size(), 2);
    EXPECT_EQ(info.getAsks().size(), 0);

    ob.addOrder(f.make(OrderType::GoodTillCancel, Side::Buy, 60, 4));
    ob.addOrder(f.make(OrderType::GoodTillCancel, Side::Buy, 61, 10));

    info = ob.getOrderInfos();
    EXPECT_EQ(ob.size(), 5);
    EXPECT_EQ(info.getBids().size(), 3);
    EXPECT_EQ(info.getAsks().size(), 0);

    auto it = info.getBids().begin();
    ASSERT_NE(it, info.getBids().end());
    EXPECT_EQ(it->price, 61);
    EXPECT_EQ(it->quantity, 10);

    ++it;
    ASSERT_NE(it, info.getBids().end());
    EXPECT_EQ(it->price, 60);
    EXPECT_EQ(it->quantity, 14);

    ++it;
    ASSERT_NE(it, info.getBids().end());
    EXPECT_EQ(it->price, 50);
    EXPECT_EQ(it->quantity, 2);
}

TEST(GoodTillCancel, AddSells_AggregatesLevels) {
    OrderFactory f;
    Orderbook ob{};

    ob.addOrder(f.make(OrderType::GoodTillCancel, Side::Sell, 50, 1));
    ob.addOrder(f.make(OrderType::GoodTillCancel, Side::Sell, 50, 1));
    ob.addOrder(f.make(OrderType::GoodTillCancel, Side::Sell, 60, 10));
    EXPECT_EQ(ob.size(), 3);

    auto info = ob.getOrderInfos();
    EXPECT_EQ(info.getAsks().size(), 2);
    EXPECT_EQ(info.getBids().size(), 0);

    ob.addOrder(f.make(OrderType::GoodTillCancel, Side::Sell, 60, 4));
    ob.addOrder(f.make(OrderType::GoodTillCancel, Side::Sell, 61, 10));

    info = ob.getOrderInfos();
    EXPECT_EQ(ob.size(), 5);
    EXPECT_EQ(info.getAsks().size(), 3);
    EXPECT_EQ(info.getBids().size(), 0);

    auto it = info.getAsks().begin();
    ASSERT_NE(it, info.getAsks().end());
    EXPECT_EQ(it->price, 50);
    EXPECT_EQ(it->quantity, 2);

    ++it;
    ASSERT_NE(it, info.getAsks().end());
    EXPECT_EQ(it->price, 60);
    EXPECT_EQ(it->quantity, 14);

    ++it;
    ASSERT_NE(it, info.getAsks().end());
    EXPECT_EQ(it->price, 61);
    EXPECT_EQ(it->quantity, 10);
}

TEST(GoodTillCancel, Matching_ProducesTradesAndUpdatesBook) {
    OrderFactory f;
    Orderbook ob{};

    ob.addOrder(f.make(0, OrderType::GoodTillCancel, Side::Sell, 50, 1));
    ob.addOrder(f.make(1, OrderType::GoodTillCancel, Side::Sell, 50, 1));
    ob.addOrder(f.make(2, OrderType::GoodTillCancel, Side::Sell, 60, 10));

    ob.addOrder(f.make(3, OrderType::GoodTillCancel, Side::Buy, 45, 10));
    EXPECT_EQ(ob.getTrades().size(), 0);

    auto info = ob.getOrderInfos();
    EXPECT_EQ(ob.size(), 4);
    EXPECT_EQ(info.getAsks().size(), 2);
    EXPECT_EQ(info.getBids().size(), 1);

    ob.addOrder(f.make(4, OrderType::GoodTillCancel, Side::Buy, 50, 2));
    ASSERT_EQ(ob.getTrades().size(), 2);
    EXPECT_TRUE(hasTradeLike(ob.getTrades(), Trade{4, 0, 50, 50, 1}));
    EXPECT_TRUE(hasTradeLike(ob.getTrades(), Trade{4, 1, 50, 50, 1}));

    EXPECT_EQ(ob.getTrades()[0].getBidId(), 4);
    EXPECT_EQ(ob.getTrades()[1].getBidId(), 4);

    EXPECT_EQ(ob.getTrades()[0].getAskId(), 0);
    EXPECT_EQ(ob.getTrades()[1].getAskId(), 1);

    info = ob.getOrderInfos();
    EXPECT_EQ(info.getAsks().size(), 1);
    EXPECT_EQ(info.getAsks().begin()->price, 60);
    EXPECT_EQ(info.getAsks().begin()->quantity, 10);
    EXPECT_EQ(info.getBids().size(), 1);
    EXPECT_EQ(ob.size(), 2);

    ob.clearTrades();

    ob.addOrder(f.make(5, OrderType::GoodTillCancel, Side::Buy, 61, 15));
    ASSERT_EQ(ob.getTrades().size(), 1);
    EXPECT_EQ(ob.getTrades()[0].getBidId(), 5);
    EXPECT_EQ(ob.getTrades()[0].getAskId(), 2);
    EXPECT_TRUE(hasTradeLike(ob.getTrades(), Trade{5, 2, 61, 60, 10}));

    info = ob.getOrderInfos();
    EXPECT_EQ(info.getAsks().size(), 0);
    EXPECT_EQ(info.getBids().size(), 2);
    EXPECT_EQ(ob.size(), 2);

    ob.clearTrades();

    ob.addOrder(f.make(6, OrderType::GoodTillCancel, Side::Sell, 61, 5));
    ASSERT_EQ(ob.getTrades().size(), 1);
    EXPECT_TRUE(hasTradeLike(ob.getTrades(), Trade{5, 6, 61, 61, 5}));

    info = ob.getOrderInfos();
    EXPECT_EQ(info.getAsks().size(), 0);
    EXPECT_EQ(info.getBids().size(), 1);
    EXPECT_EQ(ob.size(), 1);
}

TEST(GoodTillCancel, CancelingChangesBook) {
    OrderFactory f;
    Orderbook ob{};

    ob.addOrder(f.make(0, OrderType::GoodTillCancel, Side::Sell, 100, 10));
    ob.addOrder(f.make(1, OrderType::GoodTillCancel, Side::Sell, 101, 15));
    ob.addOrder(f.make(2, OrderType::GoodTillCancel, Side::Sell, 102, 30));
    ob.addOrder(f.make(3, OrderType::GoodTillCancel, Side::Sell, 102, 20));
    OrderbookLevelInfos infos = ob.getOrderInfos();

    EXPECT_EQ(50, infos.getAsks()[2].quantity);
    EXPECT_EQ(102, infos.getAsks()[2].price);
    EXPECT_EQ(10, infos.getAsks()[0].quantity);
    EXPECT_EQ(4, ob.size());

    ob.cancelOrder(3);
    infos = ob.getOrderInfos();
    EXPECT_EQ(30, infos.getAsks()[2].quantity);
    EXPECT_EQ(102, infos.getAsks()[2].price);

    ob.addOrder(f.make(3, OrderType::GoodTillCancel, Side::Sell, 102, 20));
    infos = ob.getOrderInfos();
    EXPECT_EQ(50, infos.getAsks()[2].quantity);
    EXPECT_EQ(102, infos.getAsks()[2].price);
    EXPECT_EQ(10, infos.getAsks()[0].quantity);
    EXPECT_EQ(4, ob.size());
}

TEST(GoodTillCancel, CancelingStopsPotentialTrades) {
    OrderFactory f;
    Orderbook ob{};

    ob.addOrder(f.make(0, OrderType::GoodTillCancel, Side::Sell, 100, 10));
    ob.addOrder(f.make(1, OrderType::GoodTillCancel, Side::Sell, 101, 15));
    ob.addOrder(f.make(2, OrderType::GoodTillCancel, Side::Sell, 102, 30));
    ob.addOrder(f.make(3, OrderType::GoodTillCancel, Side::Sell, 102, 20));
    ob.cancelOrder(3);

    ob.addOrder(f.make(4, OrderType::GoodTillCancel, Side::Buy, 102, 60));
    OrderbookLevelInfos infos = ob.getOrderInfos();
    EXPECT_EQ(1, ob.size());
    EXPECT_EQ(3, ob.getTrades().size());
    EXPECT_FALSE(hasTradeLike(ob.getTrades(), Trade{4, 3, 102, 102, 20}));
    EXPECT_TRUE(hasTradeLike(ob.getTrades(), Trade{4, 2, 102, 102, 30}));
    EXPECT_TRUE(hasTradeLike(ob.getTrades(), Trade{4, 1, 102, 101, 15}));
    EXPECT_TRUE(hasTradeLike(ob.getTrades(), Trade{4, 0, 102, 100, 10}));
    EXPECT_TRUE(infos.getAsks().empty());
    EXPECT_EQ(1, infos.getBids().size());


    ob.cancelOrder(4);
    infos = ob.getOrderInfos();
    EXPECT_EQ(0, ob.size());
    EXPECT_TRUE(infos.getAsks().empty());
    EXPECT_TRUE(infos.getBids().empty());
}

TEST(GoodTillCancel, ModifyOrder_UpdatesPriceAndQuantity) {
    OrderFactory f;
    Orderbook ob{};

    ob.addOrder(f.make(0, OrderType::GoodTillCancel, Side::Buy, 100, 10));
    auto info = ob.getOrderInfos();

    EXPECT_EQ(ob.size(), 1);
    ASSERT_EQ(info.getBids().size(), 1);
    EXPECT_EQ(info.getBids()[0].price, 100);
    EXPECT_EQ(info.getBids()[0].quantity, 10);

    ob.modifyOrder(OrderModify{0, Side::Buy, 105, 7});
    EXPECT_TRUE(ob.getTrades().empty());

    info = ob.getOrderInfos();
    EXPECT_EQ(ob.size(), 1);
    EXPECT_TRUE(info.getAsks().empty());
    ASSERT_EQ(info.getBids().size(), 1);

    EXPECT_EQ(info.getBids()[0].price, 105);
    EXPECT_EQ(info.getBids()[0].quantity, 7);
}

TEST(GoodTillCancel, ModifyOrder_UnknownId_DoesNothing) {
    OrderFactory f;
    Orderbook ob{};

    ob.addOrder(f.make(0, OrderType::GoodTillCancel, Side::Buy, 100, 10));
    ob.addOrder(f.make(1, OrderType::GoodTillCancel, Side::Sell, 110, 5));

    auto before = ob.getOrderInfos();
    EXPECT_EQ(ob.size(), 2);
    ASSERT_EQ(before.getBids().size(), 1);
    ASSERT_EQ(before.getAsks().size(), 1);

    ob.modifyOrder(OrderModify{999, Side::Buy, 105, 7});
    EXPECT_TRUE(ob.getTrades().empty());

    auto after = ob.getOrderInfos();
    EXPECT_EQ(ob.size(), 2);
    ASSERT_EQ(after.getBids().size(), 1);
    ASSERT_EQ(after.getAsks().size(), 1);

    EXPECT_EQ(after.getBids()[0].price, before.getBids()[0].price);
    EXPECT_EQ(after.getBids()[0].quantity, before.getBids()[0].quantity);
    EXPECT_EQ(after.getAsks()[0].price, before.getAsks()[0].price);
    EXPECT_EQ(after.getAsks()[0].quantity, before.getAsks()[0].quantity);
}

TEST(GoodTillCancel, ModifyOrder_AllowsNewTradesToHappen) {
    OrderFactory f;
    Orderbook ob{};

    ob.addOrder(f.make(0, OrderType::GoodTillCancel, Side::Buy, 100, 10));
    ob.addOrder(f.make(1, OrderType::GoodTillCancel, Side::Buy, 101, 10));

    ob.addOrder(f.make(2, OrderType::GoodTillCancel, Side::Buy, 102, 10));
    ob.addOrder(f.make(3, OrderType::GoodTillCancel, Side::Sell, 102, 30));
    EXPECT_EQ(1, ob.getTrades().size());
    EXPECT_TRUE(hasTradeLike(ob.getTrades(), {2, 3, 102, 102, 10}));
    EXPECT_EQ(3, ob.size());

    ob.clearTrades();

    ob.modifyOrder({3, Side::Sell, 100, 20});
    EXPECT_EQ(2, ob.getTrades().size());
    EXPECT_TRUE(hasTradeLike(ob.getTrades(), {0, 3, 100, 100, 10}));
    EXPECT_TRUE(hasTradeLike(ob.getTrades(), {1, 3, 101, 100, 10}));

    OrderbookLevelInfos info = ob.getOrderInfos();
    EXPECT_EQ(ob.size(), 0);
    EXPECT_TRUE(info.getAsks().empty());
    EXPECT_TRUE(info.getBids().empty());
}
