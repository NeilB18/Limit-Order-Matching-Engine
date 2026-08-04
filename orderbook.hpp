#pragma once

#include <cstdint>
#include <functional>
#include <list>
#include <map>
#include <unordered_map>
#include <vector>

#include "order.hpp"
#include "trade.hpp"

class OrderBook
{
public:
    OrderBook() {};

    std::vector<Trade> submit(Order order);

    bool cancel(OrderID orderID);

    bool bestBid(Price &out) const;
    bool bestAsk(Price &out) const;
    Quantity depthAt(OrderAction side, Price price) const;

private:
    struct OrderLocator
    {
        OrderAction side;
        Price price;
        std::list<Order>::iterator it;
    };

    using PriceLevel = std::list<Order>;
    using BidMap = std::map<Price, PriceLevel, std::greater<>>; // high → low
    using AskMap = std::map<Price, PriceLevel>;                 // low → high

    BidMap bids_;
    AskMap asks_;
    std::unordered_map<OrderID, OrderLocator> id_index_;
    Sequence next_sequence_ = 1;
};
