#include "order.hpp"
struct Trade
{
    OrderID maker_orderID_;
    OrderID taker_orderID_;
    Price price_;
    Quantity quantity_;
    Sequence sequence_;
};