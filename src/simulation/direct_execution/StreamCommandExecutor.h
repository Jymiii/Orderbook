#pragma once

#include "commands/commands.h"
#include "orderbook/Order.h"
#include "orderbook/OrderModify.h"
#include "orderbook/Orderbook.h"
#include "spsc_queue.h"

#include <atomic>

class StreamCommandExecutor
{
public:
    StreamCommandExecutor(spsc_queue<Command>& queue, Orderbook& orderbook, std::atomic<bool>& stopFlag)
        : queue_{queue}, orderbook_{orderbook}, stop_{stopFlag}
    {
    }

    StreamCommandExecutor(const StreamCommandExecutor&) = delete;
    StreamCommandExecutor& operator=(const StreamCommandExecutor&) = delete;

    void run()
    {
        Command cmd;
        while (true)
        {
            if (queue_.pop(cmd))
                execute(cmd);
            else if (stop_.load(std::memory_order_acquire))
                break;
        }
    }

private:
    spsc_queue<Command>& queue_;
    Orderbook& orderbook_;
    std::atomic<bool>& stop_;

    void execute(const Command& cmd)
    {
        std::visit(Overloaded{
            [&](const NewOrderCmd& c) { orderbook_.addOrder(Order{c.id, c.type, c.side, c.price, c.quantity}); },
            [&](const CancelOrderCmd& c) { orderbook_.cancelOrder(c.id); },
            [&](const ModifyOrderCmd& c) { orderbook_.modifyOrder(OrderModify{c.id, c.side, c.price, c.quantity}); },
                [&](const PruneGFDCmd&) { orderbook_.pruneStaleGoodForDay(); }
        }, cmd);
    }
};
