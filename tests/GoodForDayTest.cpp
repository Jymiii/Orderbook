#include "TestHelpers.h"
#include "gtest/gtest.h"

TEST(GoodForDay, PruneStaleGoodForNow_RemovesOnlyGFD) {
    OrderFactory f;
    Orderbook ob{false};

    // Add mix
    ob.addOrder(f.make(0, OrderType::GoodForDay, Side::Buy, 50, 10));
    ob.addOrder(f.make(1, OrderType::GoodTillCancel, Side::Buy, 51, 5));
    ob.addOrder(f.make(2, OrderType::GoodForDay, Side::Sell, 60, 7));
    ob.addOrder(f.make(3, OrderType::GoodTillCancel, Side::Sell, 61, 3));

    EXPECT_EQ(4, ob.size());

    PruneTestHelper::pruneStaleGoodForNow(ob);

    EXPECT_EQ(2, ob.size());  // only ids 1 and 3 remain
    auto info = ob.getOrderInfos();

    ASSERT_EQ(1, info.getBids().size());
    EXPECT_EQ(51, info.getBids()[0].price);
    EXPECT_EQ(5, info.getBids()[0].quantity);

    ASSERT_EQ(1, info.getAsks().size());
    EXPECT_EQ(61, info.getAsks()[0].price);
    EXPECT_EQ(3, info.getAsks()[0].quantity);
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
