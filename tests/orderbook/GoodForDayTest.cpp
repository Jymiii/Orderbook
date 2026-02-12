//
// Created by Jimi van der Meer on 12/02/2026.
//
#include "TestHelpers.h"

class PruneTestHelper {
public:
    static void pruneStaleGoodForNow(Orderbook &ob) {
        ob.pruneStaleGoodForNow();
    }
};

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
    EXPECT_EQ(51, info.getBids()[0].price_);
    EXPECT_EQ(5, info.getBids()[0].quantity_);

    ASSERT_EQ(1, info.getAsks().size());
    EXPECT_EQ(61, info.getAsks()[0].price_);
    EXPECT_EQ(3, info.getAsks()[0].quantity_);
}
