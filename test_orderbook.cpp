#include "orderbook.hpp"
#include <cassert>
#include <iostream>

// ---- helpers ----
static OrderID next_id = 1;
OrderID freshID() { return next_id++; }

void test_full_match_single_level()
{
    OrderBook book;
    Order sell(OrderAction::SELL, OrderType::LIMIT_ORDER, freshID(), 100, 10);
    book.submit(sell);

    Order buy(OrderAction::BUY, OrderType::LIMIT_ORDER, freshID(), 100, 10);
    auto trades = book.submit(buy);

    assert(trades.size() == 1);
    assert(trades[0].maker_orderID_ == sell.orderID());
    assert(trades[0].taker_orderID_ == buy.orderID());
    assert(trades[0].price_ == 100);
    assert(trades[0].quantity_ == 10);

    assert(book.depthAt(OrderAction::SELL, 100) == 0);
    assert(book.depthAt(OrderAction::BUY, 100) == 0);

    Price out;
    assert(!book.bestAsk(out));
    assert(!book.bestBid(out));

    std::cout << "test_full_match_single_level PASSED\n";
}

void test_partial_fill_incoming_larger()
{
    OrderBook book;
    Order sell(OrderAction::SELL, OrderType::LIMIT_ORDER, freshID(), 100, 5);
    book.submit(sell);

    Order buy(OrderAction::BUY, OrderType::LIMIT_ORDER, freshID(), 100, 10);
    auto trades = book.submit(buy);

    assert(trades.size() == 1);
    assert(trades[0].quantity_ == 5);

    assert(book.depthAt(OrderAction::SELL, 100) == 0);
    assert(book.depthAt(OrderAction::BUY, 100) == 5); // leftover rests

    std::cout << "test_partial_fill_incoming_larger PASSED\n";
}

void test_partial_fill_resting_larger()
{
    OrderBook book;
    Order sell(OrderAction::SELL, OrderType::LIMIT_ORDER, freshID(), 100, 10);
    book.submit(sell);

    Order buy(OrderAction::BUY, OrderType::LIMIT_ORDER, freshID(), 100, 3);
    auto trades = book.submit(buy);

    assert(trades.size() == 1);
    assert(trades[0].quantity_ == 3);

    assert(book.depthAt(OrderAction::SELL, 100) == 7); // resting order partially filled
    assert(book.depthAt(OrderAction::BUY, 100) == 0);  // incoming fully filled, nothing rests

    std::cout << "test_partial_fill_resting_larger PASSED\n";
}

void test_multi_level_walk()
{
    OrderBook book;
    book.submit(Order(OrderAction::SELL, OrderType::LIMIT_ORDER, freshID(), 100, 3));
    book.submit(Order(OrderAction::SELL, OrderType::LIMIT_ORDER, freshID(), 101, 4));
    book.submit(Order(OrderAction::SELL, OrderType::LIMIT_ORDER, freshID(), 102, 5));

    Order buy(OrderAction::BUY, OrderType::LIMIT_ORDER, freshID(), 101, 8);
    auto trades = book.submit(buy);

    assert(trades.size() == 2);
    assert(trades[0].price_ == 100 && trades[0].quantity_ == 3);
    assert(trades[1].price_ == 101 && trades[1].quantity_ == 4);

    assert(book.depthAt(OrderAction::SELL, 100) == 0);
    assert(book.depthAt(OrderAction::SELL, 101) == 0);
    assert(book.depthAt(OrderAction::SELL, 102) == 5); // untouched, price too high
    assert(book.depthAt(OrderAction::BUY, 101) == 1);  // 8 - 3 - 4 = 1 leftover rests

    std::cout << "test_multi_level_walk PASSED\n";
}

void test_cancel_resting()
{
    OrderBook book;
    Order buy(OrderAction::BUY, OrderType::LIMIT_ORDER, freshID(), 100, 10);
    book.submit(buy);

    assert(book.depthAt(OrderAction::BUY, 100) == 10);
    assert(book.cancel(buy.orderID()) == true);
    assert(book.depthAt(OrderAction::BUY, 100) == 0);
    assert(book.cancel(buy.orderID()) == false); // already gone

    std::cout << "test_cancel_resting PASSED\n";
}

void test_cancel_partially_filled()
{
    OrderBook book;
    Order sell(OrderAction::SELL, OrderType::LIMIT_ORDER, freshID(), 100, 5);
    book.submit(sell);

    Order buy(OrderAction::BUY, OrderType::LIMIT_ORDER, freshID(), 100, 10);
    auto trades = book.submit(buy); // 5 filled, 5 rests on bids @ 100

    assert(trades.size() == 1);
    assert(book.depthAt(OrderAction::BUY, 100) == 5);

    assert(book.cancel(buy.orderID()) == true);
    assert(book.depthAt(OrderAction::BUY, 100) == 0);
    // earlier trade still stands -- trades vector from submit() is untouched
    assert(trades[0].quantity_ == 5);

    std::cout << "test_cancel_partially_filled PASSED\n";
}

void test_market_and_limit_against_empty_book()
{
    OrderBook book;

    // Market order against empty book: nothing to match, nothing rests.
    Order marketBuy(OrderAction::BUY, OrderType::MARKET_ORDER, freshID(), 0, 10);
    auto trades = book.submit(marketBuy);
    assert(trades.empty());
    Price out;
    assert(!book.bestBid(out)); // market orders never rest

    // Limit order against empty book: rests cleanly.
    Order limitBuy(OrderAction::BUY, OrderType::LIMIT_ORDER, freshID(), 100, 5);
    trades = book.submit(limitBuy);
    assert(trades.empty());
    assert(book.bestBid(out) && out == 100);
    assert(book.depthAt(OrderAction::BUY, 100) == 5);

    std::cout << "test_market_and_limit_against_empty_book PASSED\n";
}

int main()
{
    test_full_match_single_level();
    test_partial_fill_incoming_larger();
    test_partial_fill_resting_larger();
    test_multi_level_walk();
    test_cancel_resting();
    test_cancel_partially_filled();
    test_market_and_limit_against_empty_book();

    std::cout << "\nALL TESTS PASSED\n";
    return 0;
}