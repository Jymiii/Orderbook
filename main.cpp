#include "orderbook/Orderbook.h"
#include <iostream>
#include <thread>

int main() {
    Orderbook ob{};

    ob.addOrder({1, OrderType::GoodForDay, Side::Buy, 100, 10});
    std::cout << "Before: " << ob.size() << "\n";

    // Wait long enough for your prune to trigger (only practical if close is soon)
    std::this_thread::sleep_for(std::chrono::seconds(10));

    std::cout << "After: " << ob.size() << "\n";
}
