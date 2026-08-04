#pragma once

#include <cassert>
#include <cstdint>

enum class OrderAction
{
    BUY,
    SELL
};
enum class OrderType
{
    MARKET_ORDER,
    LIMIT_ORDER
};
enum class OrderStatus
{
    RESTING,
    FILLED,
    PARTIALLY_FILLED,
    CANCELLED
};

using Price = std::int64_t;
using Quantity = std::uint32_t;
using OrderID = std::uint64_t;
using Sequence = std::uint64_t;

class Order
{
public:
    Order(OrderAction action, OrderType type, OrderID orderID, Price price, Quantity quantity)
        : action_{action},
          type_{type},
          orderID_{orderID},
          price_{price},
          initialQuantity_{quantity},
          remainingQuantity_{quantity},
          status_{OrderStatus::RESTING},
          sequence_{0}
    {
        assert(quantity > 0);
    }
    OrderAction action() const { return action_; }
    OrderID orderID() const { return orderID_; }
    OrderType type() const { return type_; }
    Price price() const { return price_; }
    OrderStatus status() const { return status_; }
    Quantity initialQuantity() const { return initialQuantity_; }
    Quantity remainingQuantity() const { return remainingQuantity_; }
    Quantity filledQuantity() const { return initialQuantity_ - remainingQuantity_; }
    Sequence sequence() const { return sequence_; }

    bool isResting() const
    {
        return status_ == OrderStatus::RESTING || status_ == OrderStatus::PARTIALLY_FILLED;
    }

    void assignSequence(Sequence s)
    {
        assert(sequence_ == 0);
        sequence_ = s;
    }

    void fill(Quantity q)
    {
        assert(q > 0);
        assert(q <= remainingQuantity_);
        assert(status_ != OrderStatus::CANCELLED);
        assert(status_ != OrderStatus::FILLED);
        remainingQuantity_ -= q;
        status_ = (remainingQuantity_ == 0) ? OrderStatus::FILLED : OrderStatus::PARTIALLY_FILLED;
    }

    void cancel()
    {
        assert(status_ != OrderStatus::FILLED);
        status_ = OrderStatus::CANCELLED;
    }

private:
    OrderAction action_;
    OrderType type_;
    OrderID orderID_;
    Price price_;
    Quantity initialQuantity_;
    Quantity remainingQuantity_;
    OrderStatus status_;
    Sequence sequence_;
};
