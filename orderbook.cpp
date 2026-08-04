#include "orderbook.hpp"
#include <algorithm>
#include <cassert>

std::vector<Trade> OrderBook::submit(Order order)
{
    std::vector<Trade> trades;
    order.assignSequence(next_sequence_++);

    const bool is_buy    = (order.action() == OrderAction::BUY);
    const bool is_market = (order.type()   == OrderType::MARKET_ORDER);

    if (is_buy)
    {
        while (order.remainingQuantity() > 0 && !asks_.empty())
        {
            auto level_it = asks_.begin();
            bool crosses = is_market || (level_it->first <= order.price());
            if (!crosses) break;

            auto &level = level_it->second;
            for (auto resting_it = level.begin();
                 resting_it != level.end() && order.remainingQuantity() > 0;)
            {
                Quantity qty = std::min(order.remainingQuantity(),
                                        resting_it->remainingQuantity());
                resting_it->fill(qty);
                order.fill(qty);
                trades.push_back({resting_it->orderID(), order.orderID(),
                                   resting_it->price(), qty, next_sequence_++});

                if (resting_it->remainingQuantity() == 0)
                {
                    id_index_.erase(resting_it->orderID());
                    resting_it = level.erase(resting_it);
                }
                else
                    ++resting_it;
            }

            if (level.empty())
                asks_.erase(level_it);
        }
    }
    else
    {
        while (order.remainingQuantity() > 0 && !bids_.empty())
        {
            auto level_it = bids_.begin();
            bool crosses = is_market || (level_it->first >= order.price());
            if (!crosses) break;

            auto &level = level_it->second;
            for (auto resting_it = level.begin();
                 resting_it != level.end() && order.remainingQuantity() > 0;)
            {
                Quantity qty = std::min(order.remainingQuantity(),
                                        resting_it->remainingQuantity());
                resting_it->fill(qty);
                order.fill(qty);
                trades.push_back({resting_it->orderID(), order.orderID(),
                                   resting_it->price(), qty, next_sequence_++});

                if (resting_it->remainingQuantity() == 0)
                {
                    id_index_.erase(resting_it->orderID());
                    resting_it = level.erase(resting_it);
                }
                else
                    ++resting_it;
            }

            if (level.empty())
                bids_.erase(level_it);
        }
    }

    // --- rest unfilled limit remainder ---
    if (order.remainingQuantity() > 0 && !is_market)
    {
        auto &level = is_buy ? bids_[order.price()] : asks_[order.price()];
        level.push_back(order);
        id_index_[order.orderID()] = {order.action(), order.price(),
                                       std::prev(level.end())};
    }

    return trades;
}

bool OrderBook::bestBid(Price &out) const
{
    if (bids_.empty())
        return false;
    out = bids_.begin()->first;
    return true;
}

bool OrderBook::bestAsk(Price &out) const
{
    if (asks_.empty())
        return false;
    out = asks_.begin()->first;
    return true;
}

Quantity OrderBook::depthAt(OrderAction side, Price price) const
{
    Quantity total = 0;
    if (side == OrderAction::BUY)
    {
        auto it = bids_.find(price);
        if (it != bids_.end())
            for (const Order &o : it->second)
                total += o.remainingQuantity();
    }
    else
    {
        auto it = asks_.find(price);
        if (it != asks_.end())
            for (const Order &o : it->second)
                total += o.remainingQuantity();
    }
    return total;
}

bool OrderBook::cancel(OrderID id)
{
    auto found = id_index_.find(id);
    if (found == id_index_.end())
        return false;

    const OrderLocator &loc = found->second;

    if (loc.side == OrderAction::BUY)
    {
        auto level_it = bids_.find(loc.price);
        assert(level_it != bids_.end());
        auto &level = level_it->second;
        level.erase(loc.it);
        if (level.empty())
        {
            bids_.erase(level_it);
        }
    }
    else
    {
        auto level_it = asks_.find(loc.price);
        assert(level_it != asks_.end());
        auto &level = level_it->second;
        level.erase(loc.it);
        if (level.empty())
        {
            asks_.erase(level_it);
        }
    }

    id_index_.erase(found);
    return true;
}