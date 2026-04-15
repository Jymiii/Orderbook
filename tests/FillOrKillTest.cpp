#include "TestHelpers.h"
#include "gtest/gtest.h"

TEST(FillOrKill, NoCounterParty) {
    OrderFactory f;
    Orderbook ob{};

    ob.addOrder(f.make(OrderType::FillOrKill, Side::Buy, 50, 1));
    ob.addOrder(f.make(OrderType::FillOrKill, Side::Sell, 50, 1));
    EXPECT_EQ(ob.size(), 0);
    auto info = ob.getOrderInfos();
    EXPECT_EQ(info.getBids().size(), 0);
    EXPECT_EQ(info.getAsks().size(), 0);
    EXPECT_TRUE((ob.getTrades().empty()));
    EXPECT_TRUE((ob.getTrades().empty()));
}

TEST(FillOrKill, CantFullyFillSimple) {
    OrderFactory f;
    Orderbook ob{};

    ob.addOrder(f.make(OrderType::GoodTillCancel, Side::Buy, 50, 10));
    ob.addOrder(f.make(OrderType::FillOrKill, Side::Sell, 50, 11));
    EXPECT_EQ(ob.size(), 1);
    auto info = ob.getOrderInfos();
    EXPECT_EQ(info.getBids().size(), 1);
    EXPECT_EQ(info.getAsks().size(), 0);
    EXPECT_TRUE((ob.getTrades().empty()));
    EXPECT_TRUE((ob.getTrades().empty()));
}

TEST(FillOrKill, CanFullyFillSimple) {
    OrderFactory f;
    Orderbook ob{};

    ob.addOrder(f.make(0, OrderType::GoodTillCancel, Side::Buy, 50, 10));
    ob.addOrder(f.make(1, OrderType::FillOrKill, Side::Sell, 50, 10));
    EXPECT_EQ(ob.size(), 0);
    auto info = ob.getOrderInfos();
    EXPECT_EQ(info.getBids().size(), 0);
    EXPECT_EQ(info.getAsks().size(), 0);
    EXPECT_EQ(1, ob.getTrades().size());
    EXPECT_TRUE(hasTradeLike(ob.getTrades(), {0, 1, 50, 50, 10}));
}

TEST(FillOrKill, CantFullyFillBig) {
    OrderFactory f;
    Orderbook ob{};

    ob.addOrder(f.make(0, OrderType::GoodTillCancel, Side::Buy, 50, 10));
    ob.addOrder(f.make(1, OrderType::GoodTillCancel, Side::Buy, 50, 10));
    ob.addOrder(f.make(2, OrderType::GoodTillCancel, Side::Buy, 52, 15));
    ob.addOrder(f.make(3, OrderType::GoodTillCancel, Side::Buy, 53, 4));
    ob.addOrder(f.make(4, OrderType::GoodTillCancel, Side::Buy, 60, 10));

    ob.addOrder(f.make(OrderType::FillOrKill, Side::Sell, 50, 50));
    EXPECT_EQ(ob.size(), 5);
    auto info = ob.getOrderInfos();
    EXPECT_EQ(info.getBids().size(), 4);
    EXPECT_EQ(info.getAsks().size(), 0);
    EXPECT_TRUE((ob.getTrades().empty()));

    ob.addOrder(f.make(OrderType::FillOrKill, Side::Sell, 51, 30));
    EXPECT_EQ(ob.size(), 5);
    info = ob.getOrderInfos();
    EXPECT_EQ(info.getBids().size(), 4);
    EXPECT_EQ(info.getAsks().size(), 0);
    EXPECT_TRUE((ob.getTrades().empty()));

    ob.addOrder(f.make(OrderType::FillOrKill, Side::Sell, 52, 30));
    EXPECT_EQ(ob.size(), 5);
    info = ob.getOrderInfos();
    EXPECT_EQ(info.getBids().size(), 4);
    EXPECT_EQ(info.getAsks().size(), 0);
    EXPECT_TRUE((ob.getTrades().empty()));

    ob.addOrder(f.make(OrderType::FillOrKill, Side::Sell, 61, 1));
    EXPECT_EQ(ob.size(), 5);
    info = ob.getOrderInfos();
    EXPECT_EQ(info.getBids().size(), 4);
    EXPECT_EQ(info.getAsks().size(), 0);
    EXPECT_TRUE((ob.getTrades().empty()));
}

TEST(FillOrKill, CanFullyFillBig) {
    OrderFactory f;
    Orderbook ob{};

    ob.addOrder(f.make(0, OrderType::GoodTillCancel, Side::Sell, 50, 10));
    ob.addOrder(f.make(1, OrderType::GoodTillCancel, Side::Sell, 50, 10));
    ob.addOrder(f.make(2, OrderType::GoodTillCancel, Side::Sell, 52, 15));
    ob.addOrder(f.make(3, OrderType::GoodTillCancel, Side::Sell, 53, 4));
    ob.addOrder(f.make(4, OrderType::GoodTillCancel, Side::Sell, 55, 10));

    ob.addOrder(f.make(5, OrderType::FillOrKill, Side::Buy, 55, 40));
    EXPECT_EQ(ob.size(), 1);
    auto info = ob.getOrderInfos();
    EXPECT_EQ(info.getBids().size(), 0);
    EXPECT_EQ(info.getAsks().size(), 1);
    EXPECT_EQ(5, ob.getTrades().size());
    EXPECT_TRUE(hasTradeLike(ob.getTrades(), {5, 0, 55, 50, 10}));
    EXPECT_TRUE(hasTradeLike(ob.getTrades(), {5, 1, 55, 50, 10}));
    EXPECT_TRUE(hasTradeLike(ob.getTrades(), {5, 2, 55, 52, 15}));
    EXPECT_TRUE(hasTradeLike(ob.getTrades(), {5, 3, 55, 53, 4}));
    EXPECT_TRUE(hasTradeLike(ob.getTrades(), {5, 4, 55, 55, 1}));
}

TEST(FillOrKill, CanFullyFillBigBuy) {
    OrderFactory f;
    Orderbook ob{};

    ob.addOrder(f.make(0, OrderType::GoodTillCancel, Side::Buy, 50, 10));
    ob.addOrder(f.make(1, OrderType::GoodTillCancel, Side::Buy, 50, 10));
    ob.addOrder(f.make(2, OrderType::GoodTillCancel, Side::Buy, 52, 15));
    ob.addOrder(f.make(3, OrderType::GoodTillCancel, Side::Buy, 53, 4));
    ob.addOrder(f.make(4, OrderType::GoodTillCancel, Side::Buy, 55, 10));

    ob.addOrder(f.make(5, OrderType::FillOrKill, Side::Sell, 50, 40));
    EXPECT_EQ(ob.size(), 1);
    auto info = ob.getOrderInfos();
    EXPECT_EQ(info.getBids().size(), 1);
    EXPECT_EQ(info.getAsks().size(), 0);
    EXPECT_EQ(5, ob.getTrades().size());
    EXPECT_TRUE(hasTradeLike(ob.getTrades(), {4, 5, 55, 50, 10}));
    EXPECT_TRUE(hasTradeLike(ob.getTrades(), {3, 5, 53, 50, 4}));
    EXPECT_TRUE(hasTradeLike(ob.getTrades(), {2, 5, 52, 50, 15}));
    // TimeOrder test
    EXPECT_TRUE(hasTradeLike(ob.getTrades(), {1, 5, 50, 50, 1}));
    EXPECT_TRUE(hasTradeLike(ob.getTrades(), {0, 5, 50, 50, 10}));
}
TEST(FillOrKill, BookUnchangedOnFailure) {
    OrderFactory f;
    Orderbook ob{};

    ob.addOrder(f.make(0, OrderType::GoodTillCancel, Side::Sell, 100, 5));
    ob.addOrder(f.make(1, OrderType::GoodTillCancel, Side::Sell, 101, 5));

    auto before = ob.getOrderInfos();
    std::size_t sizeBefore = ob.size();

    ob.addOrder(f.make(2, OrderType::FillOrKill, Side::Buy, 101, 11));

    EXPECT_TRUE(ob.getTrades().empty());
    EXPECT_EQ(sizeBefore, ob.size());
    auto after = ob.getOrderInfos();

    ASSERT_EQ(before.getAsks().size(), after.getAsks().size());
    for (std::size_t i = 0; i < before.getAsks().size(); ++i) {
        EXPECT_EQ(before.getAsks()[i].price, after.getAsks()[i].price);
        EXPECT_EQ(before.getAsks()[i].quantity, after.getAsks()[i].quantity);
    }
}

TEST(FillOrKill, ExactQuantityMatch_Succeeds) {
    OrderFactory f;
    Orderbook ob{};

    ob.addOrder(f.make(0, OrderType::GoodTillCancel, Side::Sell, 100, 7));

    ob.addOrder(f.make(1, OrderType::FillOrKill, Side::Buy, 100, 7));

    ASSERT_EQ(1, ob.getTrades().size());
    EXPECT_TRUE(hasTradeLike(ob.getTrades(), {1, 0, 100, 100, 7}));
    EXPECT_EQ(0, ob.size());
}

TEST(FillOrKill, OneUnitShort_Fails) {
    OrderFactory f;
    Orderbook ob{};

    ob.addOrder(f.make(0, OrderType::GoodTillCancel, Side::Sell, 100, 9));

    ob.addOrder(f.make(1, OrderType::FillOrKill, Side::Buy, 100, 10));

    EXPECT_TRUE(ob.getTrades().empty());
    EXPECT_EQ(1, ob.size());
    auto info = ob.getOrderInfos();
    ASSERT_EQ(1, info.getAsks().size());
    EXPECT_EQ(100, info.getAsks()[0].price);
    EXPECT_EQ(9, info.getAsks()[0].quantity);
}

TEST(FillOrKill, DoesNotRestInBook) {
    OrderFactory f;
    Orderbook ob{};

    ob.addOrder(f.make(0, OrderType::FillOrKill, Side::Buy, 50, 10));

    EXPECT_TRUE(ob.getTrades().empty());
    EXPECT_EQ(0, ob.size());
    auto info = ob.getOrderInfos();
    EXPECT_TRUE(info.getBids().empty());
    EXPECT_TRUE(info.getAsks().empty());
}

TEST(LevelData, CancelUpdatesLevelData_FillOrKillSeesCorrectQuantity) {
    OrderFactory f;
    Orderbook ob{};

    ob.addOrder(f.make(0, OrderType::GoodTillCancel, Side::Sell, 100, 10));
    ob.addOrder(f.make(1, OrderType::GoodTillCancel, Side::Sell, 100, 10));

    ob.cancelOrder(1);

    ob.addOrder(f.make(2, OrderType::FillOrKill, Side::Buy, 100, 11));
    EXPECT_TRUE(ob.getTrades().empty());

    ob.addOrder(f.make(3, OrderType::FillOrKill, Side::Buy, 100, 10));
    ASSERT_EQ(1, ob.getTrades().size());
    EXPECT_EQ(0, ob.size());
}

TEST(LevelData, PartialFillUpdatesLevelData) {
    OrderFactory f;
    Orderbook ob{};

    ob.addOrder(f.make(0, OrderType::GoodTillCancel, Side::Sell, 100, 10));

    ob.addOrder(f.make(1, OrderType::GoodTillCancel, Side::Buy, 100, 6));
    EXPECT_EQ(1, ob.getTrades().size());
    ob.addOrder(f.make(2, OrderType::FillOrKill, Side::Buy, 100, 5));
    EXPECT_EQ(1, ob.getTrades().size());

    ob.addOrder(f.make(3, OrderType::FillOrKill, Side::Buy, 100, 4));
    ASSERT_EQ(2, ob.getTrades().size());
    EXPECT_EQ(0, ob.size());
}
