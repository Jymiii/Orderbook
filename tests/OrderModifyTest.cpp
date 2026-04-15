#include "TestHelpers.h"
#include "gtest/gtest.h"

TEST(OrderModify, StoresFieldsAndConvertsToOrder) {
    constexpr OrderId id = 42;
    constexpr Side side = Side::Buy;
    constexpr Price price = 123;
    constexpr Quantity qty = 7;

    OrderModify mod{id, side, price, qty};

    EXPECT_EQ(mod.getId(), id);
    EXPECT_EQ(mod.getSide(), side);
    EXPECT_EQ(mod.getPrice(), price);
    EXPECT_EQ(mod.getQuantity(), qty);

    auto order = mod.toOrder(OrderType::GoodTillCancel);

    EXPECT_EQ(order.getId(), id);
    EXPECT_EQ(order.getSide(), side);
    EXPECT_EQ(order.getPrice(), price);
    EXPECT_EQ(order.getRemainingQuantity(), qty);
    EXPECT_EQ(order.getType(), OrderType::GoodTillCancel);
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
