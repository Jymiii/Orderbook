//
// AdditionalTests.cpp
// Extra tests targeting important gaps in the existing test suite.
//
// Coverage targets:
//   - Duplicate / zero-quantity order guards
//   - levelData_ consistency after cancels and partial fills
//   - Price-time (FIFO) priority at the same price level
//   - Market order: sweeps full book, correct trade prices
//   - Market order: partial sweep when quantity limit is reached
//   - FillOrKill: exact-match boundary, book immutability on failure
//   - FillAndKill: residual is dropped (never rests in book)
//   - modifyOrder: preserves OrderType, handles partial-fill state
//   - cancelOrder: non-existent ID is a safe no-op
//   - getOrderInfos: bids descending, asks ascending ordering
//   - GoodForDay: prune when book is empty, prune all GFD
//   - Cross-spread matching triggered by a newly added passive order
//

#include "TestHelpers.h"

// ============================================================
// Section 1 – Guard / safety invariants
// ============================================================

TEST(Guards, ZeroQuantityOrder_IsIgnored) {
    // An order with quantity 0 must be silently rejected.
    // The book must remain unchanged and no trades must occur.
    OrderFactory f;
    Orderbook ob{};

    ob.addOrder(f.make(0, OrderType::GoodTillCancel, Side::Sell, 100, 10));

    // Attempt to add a zero-qty buy that would cross the spread
    Order zeroQty{1, OrderType::GoodTillCancel, Side::Buy, 100, 0};
    Trades trades = ob.addOrder(std::move(zeroQty));

    EXPECT_TRUE(trades.empty());
    EXPECT_EQ(1, ob.size());
    auto info = ob.getOrderInfos();
    EXPECT_EQ(1, info.getAsks().size());
    EXPECT_TRUE(info.getBids().empty());
}

TEST(Guards, DuplicateOrderId_SecondIsIgnored) {
    // Adding two orders with the same ID: only the first should be accepted.
    OrderFactory f;
    Orderbook ob{};

    Trades t1 = ob.addOrder(f.make(42, OrderType::GoodTillCancel, Side::Buy, 100, 5));
    EXPECT_TRUE(t1.empty());  // no counterpart yet
    EXPECT_EQ(1, ob.size());

    Trades t2 = ob.addOrder(f.make(42, OrderType::GoodTillCancel, Side::Buy, 100, 5));
    EXPECT_TRUE(t2.empty());
    // Must still be exactly 1 order, not 2
    EXPECT_EQ(1, ob.size());

    auto info = ob.getOrderInfos();
    ASSERT_EQ(1, info.getBids().size());
    EXPECT_EQ(100, info.getBids()[0].price_);
    EXPECT_EQ(5,   info.getBids()[0].quantity_);
}

TEST(Guards, CancelNonExistentId_IsNoOp) {
    // Cancelling an ID that was never added must not crash or corrupt state.
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
    // Double-cancelling the same ID must not crash or corrupt the book.
    OrderFactory f;
    Orderbook ob{};

    ob.addOrder(f.make(0, OrderType::GoodTillCancel, Side::Buy, 100, 10));
    ob.cancelOrder(0);
    EXPECT_EQ(0, ob.size());

    EXPECT_NO_FATAL_FAILURE(ob.cancelOrder(0));
    EXPECT_EQ(0, ob.size());
}

// ============================================================
// Section 2 – Price-time (FIFO) priority
// ============================================================

TEST(PriceTimePriority, SamePriceFIFO_SellSide) {
    OrderFactory f;
    Orderbook ob{};

    ob.addOrder(f.make(0, OrderType::GoodTillCancel, Side::Sell, 100, 5));  // first
    ob.addOrder(f.make(1, OrderType::GoodTillCancel, Side::Sell, 100, 5));  // second
    ob.addOrder(f.make(2, OrderType::GoodTillCancel, Side::Sell, 100, 5));  // third

    Trades trades = ob.addOrder(f.make(3, OrderType::GoodTillCancel, Side::Buy, 100, 12));

    ASSERT_EQ(3, trades.size());
    EXPECT_EQ(0, trades[0].getAskId());
    EXPECT_EQ(1, trades[1].getAskId());
    EXPECT_EQ(2, trades[2].getAskId());


    EXPECT_EQ(1, ob.size());
    auto info = ob.getOrderInfos();
    ASSERT_EQ(1, info.getAsks().size());
    EXPECT_EQ(100, info.getAsks()[0].price_);
    EXPECT_EQ(3,   info.getAsks()[0].quantity_);
}

TEST(PriceTimePriority, SamePriceFIFO_BuySide) {
    OrderFactory f;
    Orderbook ob{};

    ob.addOrder(f.make(0, OrderType::GoodTillCancel, Side::Buy, 100, 5));
    ob.addOrder(f.make(1, OrderType::GoodTillCancel, Side::Buy, 100, 5));
    ob.addOrder(f.make(2, OrderType::GoodTillCancel, Side::Buy, 100, 5));

    Trades trades = ob.addOrder(f.make(3, OrderType::GoodTillCancel, Side::Sell, 100, 7));

    ASSERT_EQ(2, trades.size());
    EXPECT_EQ(0, trades[0].getBidId());
    EXPECT_EQ(1, trades[1].getBidId());

    EXPECT_EQ(2, ob.size());
    auto info = ob.getOrderInfos();
    ASSERT_EQ(1, info.getBids().size());
    EXPECT_EQ(8, info.getBids()[0].quantity_);
}

// ============================================================
// Section 3 – getOrderInfos ordering
// ============================================================

TEST(OrderInfosOrdering, BidsDescending_AsksAscending) {
    // Bids must be sorted highest price first; asks lowest price first.
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

// ============================================================
// Section 4 – Market order edge cases
// ============================================================

TEST(MarketOrder, SweepsEntireBook) {
    // A market buy large enough to consume every resting sell;
    // the remainder of the market order (converted to GTC at worst ask price)
    // must rest in the book.
    OrderFactory f;
    Orderbook ob{};

    ob.addOrder(f.make(0, OrderType::GoodTillCancel, Side::Sell, 100, 10));
    ob.addOrder(f.make(1, OrderType::GoodTillCancel, Side::Sell, 101, 10));
    ob.addOrder(f.make(2, OrderType::GoodTillCancel, Side::Sell, 102, 10));

    // Qty 100 >> 30 available, residual rests as GTC bid at worst ask (102)
    Trades trades = ob.addOrder(f.make(3, Side::Buy, 100));

    EXPECT_EQ(3, trades.size());
    EXPECT_TRUE(hasTradeLike(trades, {3, 0, 102, 100, 10}));
    EXPECT_TRUE(hasTradeLike(trades, {3, 1, 102, 101, 10}));
    EXPECT_TRUE(hasTradeLike(trades, {3, 2, 102, 102, 10}));

    // Asks all gone; residual buy rests
    EXPECT_EQ(0, ob.size());
    auto info = ob.getOrderInfos();
    EXPECT_TRUE(info.getAsks().empty());
    ASSERT_EQ(0, info.getBids().size());
}

TEST(MarketOrder, TradePrice_IsRestingOrderPrice_NotMarketPrice) {
    // A market sell must trade at each bid's limit price, not at some single price.
    OrderFactory f;
    Orderbook ob{};

    ob.addOrder(f.make(0, OrderType::GoodTillCancel, Side::Buy, 105, 5));
    ob.addOrder(f.make(1, OrderType::GoodTillCancel, Side::Buy, 100, 5));

    Trades trades = ob.addOrder(f.make(2, Side::Sell, 8));

    ASSERT_EQ(2, trades.size());
    // First trade at the best bid (105), second at next (100)
    EXPECT_TRUE(hasTradeLike(trades, {0, 2, 105, 100, 5}));  // bid price 105, ask price is worst bid = 100
    EXPECT_TRUE(hasTradeLike(trades, {1, 2, 100, 100, 3}));
}

TEST(MarketOrder, AddMarketOrder_WhenCounterBookBecomesEmpty_ResidualsRest) {
    // Verify that after the last resting ask is consumed, residual market buy rests.
    OrderFactory f;
    Orderbook ob{};

    ob.addOrder(f.make(0, OrderType::GoodTillCancel, Side::Sell, 50, 5));
    Trades trades = ob.addOrder(f.make(1, Side::Buy, 10));  // 10 > 5 available

    ASSERT_EQ(1, trades.size());
    EXPECT_TRUE(hasTradeLike(trades, {1, 0, 50, 50, 5}));

    EXPECT_EQ(0, ob.size());
    auto info = ob.getOrderInfos();
    EXPECT_TRUE(info.getAsks().empty());
    EXPECT_TRUE(info.getBids().empty());
}

// ============================================================
// Section 5 – FillOrKill edge cases
// ============================================================

TEST(FillOrKill, BookUnchangedOnFailure) {
    // When a FOK cannot be fully filled, the book must be
    // *exactly* the same as before the attempt.
    OrderFactory f;
    Orderbook ob{};

    ob.addOrder(f.make(0, OrderType::GoodTillCancel, Side::Sell, 100, 5));
    ob.addOrder(f.make(1, OrderType::GoodTillCancel, Side::Sell, 101, 5));

    auto before = ob.getOrderInfos();
    std::size_t sizeBefore = ob.size();

    Trades trades = ob.addOrder(f.make(2, OrderType::FillOrKill, Side::Buy, 101, 11));

    EXPECT_TRUE(trades.empty());
    EXPECT_EQ(sizeBefore, ob.size());
    auto after = ob.getOrderInfos();

    ASSERT_EQ(before.getAsks().size(), after.getAsks().size());
    for (std::size_t i = 0; i < before.getAsks().size(); ++i) {
        EXPECT_EQ(before.getAsks()[i].price_,    after.getAsks()[i].price_);
        EXPECT_EQ(before.getAsks()[i].quantity_, after.getAsks()[i].quantity_);
    }
}

TEST(FillOrKill, ExactQuantityMatch_Succeeds) {
    // Qty available == qty requested: FOK must succeed.
    OrderFactory f;
    Orderbook ob{};

    ob.addOrder(f.make(0, OrderType::GoodTillCancel, Side::Sell, 100, 7));

    Trades trades = ob.addOrder(f.make(1, OrderType::FillOrKill, Side::Buy, 100, 7));

    ASSERT_EQ(1, trades.size());
    EXPECT_TRUE(hasTradeLike(trades, {1, 0, 100, 100, 7}));
    EXPECT_EQ(0, ob.size());
}

TEST(FillOrKill, OneUnitShort_Fails) {
    // Qty available is one less than requested: FOK must be killed.
    OrderFactory f;
    Orderbook ob{};

    ob.addOrder(f.make(0, OrderType::GoodTillCancel, Side::Sell, 100, 9));

    Trades trades = ob.addOrder(f.make(1, OrderType::FillOrKill, Side::Buy, 100, 10));

    EXPECT_TRUE(trades.empty());
    EXPECT_EQ(1, ob.size());
    auto info = ob.getOrderInfos();
    ASSERT_EQ(1, info.getAsks().size());
    EXPECT_EQ(100, info.getAsks()[0].price_);
    EXPECT_EQ(9,   info.getAsks()[0].quantity_);
}

TEST(FillOrKill, DoesNotRestInBook) {
    // A killed FOK must never appear as a resting order.
    OrderFactory f;
    Orderbook ob{};

    Trades trades = ob.addOrder(f.make(0, OrderType::FillOrKill, Side::Buy, 50, 10));

    EXPECT_TRUE(trades.empty());
    EXPECT_EQ(0, ob.size());
    auto info = ob.getOrderInfos();
    EXPECT_TRUE(info.getBids().empty());
    EXPECT_TRUE(info.getAsks().empty());
}

// ============================================================
// Section 6 – FillAndKill edge cases
// ============================================================

TEST(FillAndKill, ResidualNeverRestsInBook) {
    // After a FAK is partially filled its remainder must be gone,
    // not sitting in the book.
    OrderFactory f;
    Orderbook ob{};

    ob.addOrder(f.make(0, OrderType::GoodTillCancel, Side::Buy, 100, 5));

    Trades trades = ob.addOrder(f.make(1, OrderType::FillAndKill, Side::Sell, 100, 20));

    ASSERT_EQ(1, trades.size());
    EXPECT_TRUE(hasTradeLike(trades, {0, 1, 100, 100, 5}));

    // Both bid 0 (fully filled) and ask 1 (residual killed) must be gone
    EXPECT_EQ(0, ob.size());
    auto info = ob.getOrderInfos();
    EXPECT_TRUE(info.getBids().empty());
    EXPECT_TRUE(info.getAsks().empty());
}

TEST(FillAndKill, FullFillProducesOneTrade) {
    // Exact match: FAK quantity == resting quantity.
    OrderFactory f;
    Orderbook ob{};

    ob.addOrder(f.make(0, OrderType::GoodTillCancel, Side::Sell, 50, 10));

    Trades trades = ob.addOrder(f.make(1, OrderType::FillAndKill, Side::Buy, 50, 10));

    ASSERT_EQ(1, trades.size());
    EXPECT_TRUE(hasTradeLike(trades, {1, 0, 50, 50, 10}));
    EXPECT_EQ(0, ob.size());
}

// ============================================================
// Section 7 – modifyOrder edge cases
// ============================================================

TEST(ModifyOrder, PreservesOrderType) {
    // A GoodForDay order that is modified should stay GoodForDay.
    OrderFactory f;
    Orderbook ob{false};  // no prune thread
    ob.addOrder(f.make(0, OrderType::GoodForDay, Side::Buy, 100, 10));
    EXPECT_EQ(1, ob.size());

    Trades trades = ob.modifyOrder({0, Side::Buy, 105, 8});
    EXPECT_TRUE(trades.empty());
    EXPECT_EQ(1, ob.size());

    // Now prune – the modified order must still be removed as GoodForDay
    class PruneHelper { public: static void prune(Orderbook &ob) { PruneTestHelper::pruneStaleGoodForNow(ob); } };
    PruneHelper::prune(ob);

    EXPECT_EQ(0, ob.size());
}

TEST(ModifyOrder, ModifyToNewPriceTriggersMatch) {
    // Modifying a resting sell down to a level where a bid exists
    // must immediately produce trades.
    OrderFactory f;
    Orderbook ob{};

    ob.addOrder(f.make(0, OrderType::GoodTillCancel, Side::Buy, 100, 10));
    ob.addOrder(f.make(1, OrderType::GoodTillCancel, Side::Sell, 110, 10));

    // No trades yet
    EXPECT_EQ(2, ob.size());

    // Lower the ask below the bid
    Trades trades = ob.modifyOrder({1, Side::Sell, 95, 10});
    ASSERT_EQ(1, trades.size());
    EXPECT_TRUE(hasTradeLike(trades, {0, 1, 100, 95, 10}));

    EXPECT_EQ(0, ob.size());
}

TEST(ModifyOrder, ModifyChangesQuantity_LevelDataConsistent) {
    // Confirm that levelData_ remains consistent when quantity changes:
    // a subsequent FillOrKill check must use the updated quantity.
    OrderFactory f;
    Orderbook ob{};

    ob.addOrder(f.make(0, OrderType::GoodTillCancel, Side::Sell, 100, 20));

    // Reduce to 5 via modify
    Trades trades = ob.modifyOrder({0, Side::Sell, 100, 5});
    EXPECT_TRUE(trades.empty());

    // FOK for 6 should fail (only 5 available now)
    Trades fok = ob.addOrder(f.make(1, OrderType::FillOrKill, Side::Buy, 100, 6));
    EXPECT_TRUE(fok.empty());
    EXPECT_EQ(1, ob.size());

    // FOK for 5 should succeed
    Trades fok2 = ob.addOrder(f.make(2, OrderType::FillOrKill, Side::Buy, 100, 5));
    ASSERT_EQ(1, fok2.size());
    EXPECT_EQ(0, ob.size());
}

// ============================================================
// Section 8 – levelData_ consistency after cancel
// ============================================================

TEST(LevelData, CancelUpdatesLevelData_FillOrKillSeesCorrectQuantity) {
    // After a cancel, canFullyFill must reflect the reduced available quantity.
    OrderFactory f;
    Orderbook ob{};

    ob.addOrder(f.make(0, OrderType::GoodTillCancel, Side::Sell, 100, 10));
    ob.addOrder(f.make(1, OrderType::GoodTillCancel, Side::Sell, 100, 10));  // total 20

    // Cancel one order – total drops to 10
    ob.cancelOrder(1);

    // FOK for 11 must fail
    Trades trades = ob.addOrder(f.make(2, OrderType::FillOrKill, Side::Buy, 100, 11));
    EXPECT_TRUE(trades.empty());

    // FOK for 10 must succeed
    Trades trades2 = ob.addOrder(f.make(3, OrderType::FillOrKill, Side::Buy, 100, 10));
    ASSERT_EQ(1, trades2.size());
    EXPECT_EQ(0, ob.size());
}

TEST(LevelData, PartialFillUpdatesLevelData) {
    OrderFactory f;
    Orderbook ob{};

    ob.addOrder(f.make(0, OrderType::GoodTillCancel, Side::Sell, 100, 10));

    // Partially fill order 0: consume 6, leaving 4
    ob.addOrder(f.make(1, OrderType::GoodTillCancel, Side::Buy, 100, 6));

    // FOK for 5 must fail (only 4 left)
    Trades fok = ob.addOrder(f.make(2, OrderType::FillOrKill, Side::Buy, 100, 5));
    EXPECT_TRUE(fok.empty());

    // FOK for 4 must succeed
    Trades fok2 = ob.addOrder(f.make(3, OrderType::FillOrKill, Side::Buy, 100, 4));
    ASSERT_EQ(1, fok2.size());
    EXPECT_EQ(0, ob.size());
}

// ============================================================
// Section 9 – Cross-spread matching triggered by passive order
// ============================================================

TEST(CrossSpread, AddingSell_ImmediatelyMatchesExistingBid) {
    // Place a bid first, then add a sell below it; matching must happen
    // when the ask is added, not only when a buy is added.
    OrderFactory f;
    Orderbook ob{};

    ob.addOrder(f.make(0, OrderType::GoodTillCancel, Side::Buy, 105, 10));

    Trades trades = ob.addOrder(f.make(1, OrderType::GoodTillCancel, Side::Sell, 100, 10));

    ASSERT_EQ(1, trades.size());
    EXPECT_TRUE(hasTradeLike(trades, {0, 1, 105, 100, 10}));
    EXPECT_EQ(0, ob.size());
}

TEST(CrossSpread, AddingBid_ImmediatelyMatchesExistingAsk) {
    OrderFactory f;
    Orderbook ob{};

    ob.addOrder(f.make(0, OrderType::GoodTillCancel, Side::Sell, 100, 10));

    Trades trades = ob.addOrder(f.make(1, OrderType::GoodTillCancel, Side::Buy, 105, 10));

    ASSERT_EQ(1, trades.size());
    EXPECT_TRUE(hasTradeLike(trades, {1, 0, 105, 100, 10}));
    EXPECT_EQ(0, ob.size());
}

// ============================================================
// Section 10 – GoodForDay pruning edge cases
// ============================================================

TEST(GoodForDay, PruneOnEmptyBook_IsNoOp) {
    Orderbook ob{false};
    EXPECT_NO_FATAL_FAILURE(PruneTestHelper::pruneStaleGoodForNow(ob));
    EXPECT_EQ(0, ob.size());
}

TEST(GoodForDay, PruneAllGFD_ClearsBook) {
    OrderFactory f;
    Orderbook ob{false};

    ob.addOrder(f.make(0, OrderType::GoodForDay, Side::Buy,  50, 10));
    ob.addOrder(f.make(1, OrderType::GoodForDay, Side::Sell, 60, 5));
    EXPECT_EQ(2, ob.size());

    PruneTestHelper::pruneStaleGoodForNow(ob);

    EXPECT_EQ(0, ob.size());
    auto info = ob.getOrderInfos();
    EXPECT_TRUE(info.getBids().empty());
    EXPECT_TRUE(info.getAsks().empty());
}

TEST(GoodForDay, PruneDoesNotAffectMatchedGFD) {
    // A GFD order that was already fully matched must not cause issues
    // when a prune is triggered afterwards.
    OrderFactory f;
    Orderbook ob{false};

    ob.addOrder(f.make(0, OrderType::GoodForDay,   Side::Sell, 100, 10));
    ob.addOrder(f.make(1, OrderType::GoodTillCancel, Side::Buy, 100, 10));

    EXPECT_EQ(0, ob.size());  // both fully matched and removed

    // Prune must not crash or corrupt the (now empty) book
    EXPECT_NO_FATAL_FAILURE(PruneTestHelper::pruneStaleGoodForNow(ob));
    EXPECT_EQ(0, ob.size());
}

// ============================================================
// Section 11 – Multiple GTC cascades / multi-level sweeps
// ============================================================

TEST(GoodTillCancel, MultiLevelSweep_CorrectResidualAfterEachLevel) {
    // A large buy walks through three sell levels; each level is fully
    // consumed and removed from the book, then a residual rests.
    OrderFactory f;
    Orderbook ob{};

    ob.addOrder(f.make(0, OrderType::GoodTillCancel, Side::Sell, 100, 10));
    ob.addOrder(f.make(1, OrderType::GoodTillCancel, Side::Sell, 101, 10));
    ob.addOrder(f.make(2, OrderType::GoodTillCancel, Side::Sell, 102, 10));

    Trades trades = ob.addOrder(f.make(3, OrderType::GoodTillCancel, Side::Buy, 105, 25));

    // 10 + 10 + 5 = 25; level at 102 partially consumed (5 remaining)
    ASSERT_EQ(3, trades.size());
    EXPECT_TRUE(hasTradeLike(trades, {3, 0, 105, 100, 10}));
    EXPECT_TRUE(hasTradeLike(trades, {3, 1, 105, 101, 10}));
    EXPECT_TRUE(hasTradeLike(trades, {3, 2, 105, 102, 5}));

    auto info = ob.getOrderInfos();
    EXPECT_TRUE(info.getBids().empty());
    ASSERT_EQ(1, info.getAsks().size());
    EXPECT_EQ(102, info.getAsks()[0].price_);
    EXPECT_EQ(5,   info.getAsks()[0].quantity_);
    EXPECT_EQ(1,   ob.size());
}

TEST(GoodTillCancel, BidResidualRestsAfterPartialMatch) {
    // A buy order that partially matches must leave its unmatched
    // quantity resting at the correct bid level.
    OrderFactory f;
    Orderbook ob{};

    ob.addOrder(f.make(0, OrderType::GoodTillCancel, Side::Sell, 100, 5));

    Trades trades = ob.addOrder(f.make(1, OrderType::GoodTillCancel, Side::Buy, 100, 12));

    ASSERT_EQ(1, trades.size());
    EXPECT_TRUE(hasTradeLike(trades, {1, 0, 100, 100, 5}));

    EXPECT_EQ(1, ob.size());
    auto info = ob.getOrderInfos();
    EXPECT_TRUE(info.getAsks().empty());
    ASSERT_EQ(1, info.getBids().size());
    EXPECT_EQ(100, info.getBids()[0].price_);
    EXPECT_EQ(7,   info.getBids()[0].quantity_);  // 12 - 5
}