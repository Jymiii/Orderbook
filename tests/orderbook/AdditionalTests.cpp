#include "TestHelpers.h"

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
    EXPECT_EQ(100, info.getBids()[0].price_);
    EXPECT_EQ(5, info.getBids()[0].quantity_);
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


TEST(PriceTimePriority, SamePriceFIFO_SellSide) {
    OrderFactory f;
    Orderbook ob{};

    ob.addOrder(f.make(0, OrderType::GoodTillCancel, Side::Sell, 100, 5));
    ob.addOrder(f.make(1, OrderType::GoodTillCancel, Side::Sell, 100, 5));
    ob.addOrder(f.make(2, OrderType::GoodTillCancel, Side::Sell, 100, 5));

    ob.addOrder(f.make(3, OrderType::GoodTillCancel, Side::Buy, 100, 12));

    ASSERT_EQ(3, ob.getTrades().size());
    EXPECT_EQ(0, ob.getTrades()[0].getAskId());
    EXPECT_EQ(1, ob.getTrades()[1].getAskId());
    EXPECT_EQ(2, ob.getTrades()[2].getAskId());


    EXPECT_EQ(1, ob.size());
    auto info = ob.getOrderInfos();
    ASSERT_EQ(1, info.getAsks().size());
    EXPECT_EQ(100, info.getAsks()[0].price_);
    EXPECT_EQ(3, info.getAsks()[0].quantity_);
}

TEST(PriceTimePriority, SamePriceFIFO_BuySide) {
    OrderFactory f;
    Orderbook ob{};

    ob.addOrder(f.make(0, OrderType::GoodTillCancel, Side::Buy, 100, 5));
    ob.addOrder(f.make(1, OrderType::GoodTillCancel, Side::Buy, 100, 5));
    ob.addOrder(f.make(2, OrderType::GoodTillCancel, Side::Buy, 100, 5));

    ob.addOrder(f.make(3, OrderType::GoodTillCancel, Side::Sell, 100, 7));

    ASSERT_EQ(2, ob.getTrades().size());
    EXPECT_EQ(0, ob.getTrades()[0].getBidId());
    EXPECT_EQ(1, ob.getTrades()[1].getBidId());

    EXPECT_EQ(2, ob.size());
    auto info = ob.getOrderInfos();
    ASSERT_EQ(1, info.getBids().size());
    EXPECT_EQ(8, info.getBids()[0].quantity_);
}


TEST(OrderInfosOrdering, BidsDescending_AsksAscending) {
    OrderFactory f;
    Orderbook ob{};

    ob.addOrder(f.make(0, OrderType::GoodTillCancel, Side::Buy, 90, 1));
    ob.addOrder(f.make(1, OrderType::GoodTillCancel, Side::Buy, 95, 1));
    ob.addOrder(f.make(2, OrderType::GoodTillCancel, Side::Buy, 80, 1));

    ob.addOrder(f.make(3, OrderType::GoodTillCancel, Side::Sell, 110, 1));
    ob.addOrder(f.make(4, OrderType::GoodTillCancel, Side::Sell, 100, 1));
    ob.addOrder(f.make(5, OrderType::GoodTillCancel, Side::Sell, 120, 1));

    auto info = ob.getOrderInfos();

    ASSERT_EQ(3, info.getBids().size());
    EXPECT_EQ(95, info.getBids()[0].price_);
    EXPECT_EQ(90, info.getBids()[1].price_);
    EXPECT_EQ(80, info.getBids()[2].price_);

    ASSERT_EQ(3, info.getAsks().size());
    EXPECT_EQ(100, info.getAsks()[0].price_);
    EXPECT_EQ(110, info.getAsks()[1].price_);
    EXPECT_EQ(120, info.getAsks()[2].price_);
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
        EXPECT_EQ(before.getAsks()[i].price_, after.getAsks()[i].price_);
        EXPECT_EQ(before.getAsks()[i].quantity_, after.getAsks()[i].quantity_);
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
    EXPECT_EQ(100, info.getAsks()[0].price_);
    EXPECT_EQ(9, info.getAsks()[0].quantity_);
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


TEST(FillAndKill, ResidualNeverRestsInBook) {
    OrderFactory f;
    Orderbook ob{};

    ob.addOrder(f.make(0, OrderType::GoodTillCancel, Side::Buy, 100, 5));

    ob.addOrder(f.make(1, OrderType::FillAndKill, Side::Sell, 100, 20));

    ASSERT_EQ(1, ob.getTrades().size());
    EXPECT_TRUE(hasTradeLike(ob.getTrades(), {0, 1, 100, 100, 5}));

    EXPECT_EQ(0, ob.size());
    auto info = ob.getOrderInfos();
    EXPECT_TRUE(info.getBids().empty());
    EXPECT_TRUE(info.getAsks().empty());
}

TEST(FillAndKill, FullFillProducesOneTrade) {
    OrderFactory f;
    Orderbook ob{};

    ob.addOrder(f.make(0, OrderType::GoodTillCancel, Side::Sell, 50, 10));

    ob.addOrder(f.make(1, OrderType::FillAndKill, Side::Buy, 50, 10));

    ASSERT_EQ(1, ob.getTrades().size());
    EXPECT_TRUE(hasTradeLike(ob.getTrades(), {1, 0, 50, 50, 10}));
    EXPECT_EQ(0, ob.size());
}


TEST(ModifyOrder, PreservesOrderType) {
    OrderFactory f;
    Orderbook ob{false};
    ob.addOrder(f.make(0, OrderType::GoodForDay, Side::Buy, 100, 10));
    EXPECT_EQ(1, ob.size());

    ob.modifyOrder({0, Side::Buy, 105, 8});
    EXPECT_TRUE(ob.getTrades().empty());
    EXPECT_EQ(1, ob.size());

    class PruneHelper {
    public:
        static void prune(Orderbook &ob) { PruneTestHelper::pruneStaleGoodForNow(ob); }
    };
    PruneHelper::prune(ob);

    EXPECT_EQ(0, ob.size());
}

TEST(ModifyOrder, ModifyToNewPriceTriggersMatch) {
    OrderFactory f;
    Orderbook ob{};

    ob.addOrder(f.make(0, OrderType::GoodTillCancel, Side::Buy, 100, 10));
    ob.addOrder(f.make(1, OrderType::GoodTillCancel, Side::Sell, 110, 10));

    EXPECT_EQ(2, ob.size());

    ob.modifyOrder({1, Side::Sell, 95, 10});
    ASSERT_EQ(1, ob.getTrades().size());
    EXPECT_TRUE(hasTradeLike(ob.getTrades(), {0, 1, 100, 95, 10}));

    EXPECT_EQ(0, ob.size());
}

TEST(ModifyOrder, ModifyChangesQuantity_LevelDataConsistent) {
    OrderFactory f;
    Orderbook ob{};

    ob.addOrder(f.make(0, OrderType::GoodTillCancel, Side::Sell, 100, 20));

    ob.modifyOrder({0, Side::Sell, 100, 5});
    EXPECT_TRUE(ob.getTrades().empty());

    ob.addOrder(f.make(1, OrderType::FillOrKill, Side::Buy, 100, 6));
    EXPECT_TRUE(ob.getTrades().empty());
    EXPECT_EQ(1, ob.size());

    ob.addOrder(f.make(2, OrderType::FillOrKill, Side::Buy, 100, 5));
    ASSERT_EQ(1, ob.getTrades().size());
    EXPECT_EQ(0, ob.size());
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


TEST(CrossSpread, AddingSell_ImmediatelyMatchesExistingBid) {
    OrderFactory f;
    Orderbook ob{};

    ob.addOrder(f.make(0, OrderType::GoodTillCancel, Side::Buy, 105, 10));

    ob.addOrder(f.make(1, OrderType::GoodTillCancel, Side::Sell, 100, 10));

    ASSERT_EQ(1, ob.getTrades().size());
    EXPECT_TRUE(hasTradeLike(ob.getTrades(), {0, 1, 105, 100, 10}));
    EXPECT_EQ(0, ob.size());
}

TEST(CrossSpread, AddingBid_ImmediatelyMatchesExistingAsk) {
    OrderFactory f;
    Orderbook ob{};

    ob.addOrder(f.make(0, OrderType::GoodTillCancel, Side::Sell, 100, 10));

    ob.addOrder(f.make(1, OrderType::GoodTillCancel, Side::Buy, 105, 10));

    ASSERT_EQ(1, ob.getTrades().size());
    EXPECT_TRUE(hasTradeLike(ob.getTrades(), {1, 0, 105, 100, 10}));
    EXPECT_EQ(0, ob.size());
}


TEST(GoodForDay, PruneOnEmptyBook_IsNoOp) {
    Orderbook ob{false};
    EXPECT_NO_FATAL_FAILURE(PruneTestHelper::pruneStaleGoodForNow(ob));
    EXPECT_EQ(0, ob.size());
}

TEST(GoodForDay, PruneAllGFD_ClearsBook) {
    OrderFactory f;
    Orderbook ob{false};

    ob.addOrder(f.make(0, OrderType::GoodForDay, Side::Buy, 50, 10));
    ob.addOrder(f.make(1, OrderType::GoodForDay, Side::Sell, 60, 5));
    EXPECT_EQ(2, ob.size());

    PruneTestHelper::pruneStaleGoodForNow(ob);

    EXPECT_EQ(0, ob.size());
    auto info = ob.getOrderInfos();
    EXPECT_TRUE(info.getBids().empty());
    EXPECT_TRUE(info.getAsks().empty());
}

TEST(GoodForDay, PruneDoesNotAffectMatchedGFD) {
    OrderFactory f;
    Orderbook ob{false};

    ob.addOrder(f.make(0, OrderType::GoodForDay, Side::Sell, 100, 10));
    ob.addOrder(f.make(1, OrderType::GoodTillCancel, Side::Buy, 100, 10));

    EXPECT_EQ(0, ob.size());

    EXPECT_NO_FATAL_FAILURE(PruneTestHelper::pruneStaleGoodForNow(ob));
    EXPECT_EQ(0, ob.size());
}


TEST(GoodTillCancel, MultiLevelSweep_CorrectResidualAfterEachLevel) {
    OrderFactory f;
    Orderbook ob{};

    ob.addOrder(f.make(0, OrderType::GoodTillCancel, Side::Sell, 100, 10));
    ob.addOrder(f.make(1, OrderType::GoodTillCancel, Side::Sell, 101, 10));
    ob.addOrder(f.make(2, OrderType::GoodTillCancel, Side::Sell, 102, 10));

    ob.addOrder(f.make(3, OrderType::GoodTillCancel, Side::Buy, 105, 25));

    ASSERT_EQ(3, ob.getTrades().size());
    EXPECT_TRUE(hasTradeLike(ob.getTrades(), {3, 0, 105, 100, 10}));
    EXPECT_TRUE(hasTradeLike(ob.getTrades(), {3, 1, 105, 101, 10}));
    EXPECT_TRUE(hasTradeLike(ob.getTrades(), {3, 2, 105, 102, 5}));

    auto info = ob.getOrderInfos();
    EXPECT_TRUE(info.getBids().empty());
    ASSERT_EQ(1, info.getAsks().size());
    EXPECT_EQ(102, info.getAsks()[0].price_);
    EXPECT_EQ(5, info.getAsks()[0].quantity_);
    EXPECT_EQ(1, ob.size());
}

TEST(GoodTillCancel, BidResidualRestsAfterPartialMatch) {
    OrderFactory f;
    Orderbook ob{};

    ob.addOrder(f.make(0, OrderType::GoodTillCancel, Side::Sell, 100, 5));

    ob.addOrder(f.make(1, OrderType::GoodTillCancel, Side::Buy, 100, 12));

    ASSERT_EQ(1, ob.getTrades().size());
    EXPECT_TRUE(hasTradeLike(ob.getTrades(), {1, 0, 100, 100, 5}));

    EXPECT_EQ(1, ob.size());
    auto info = ob.getOrderInfos();
    EXPECT_TRUE(info.getAsks().empty());
    ASSERT_EQ(1, info.getBids().size());
    EXPECT_EQ(100, info.getBids()[0].price_);
    EXPECT_EQ(7, info.getBids()[0].quantity_);
}