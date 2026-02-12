#include "orderbook/Orderbook.h"
#include <iostream>
#include <thread>
#include <chrono> // for std::chrono functions

class Timer
{
private:
    // Type aliases to make accessing nested type easier
    using Clock = std::chrono::steady_clock;
    using Second = std::chrono::duration<double, std::ratio<1> >;

    std::chrono::time_point<Clock> m_beg { Clock::now() };

public:
    void reset()
    {
        m_beg = Clock::now();
    }

    double elapsed() const
    {
        return std::chrono::duration_cast<Second>(Clock::now() - m_beg).count();
    }
};


int main() {
    Orderbook ob{};
    Timer timer;
    for (int i = 0; i < 10000000; i++){
        ob.addOrder(Order{static_cast<OrderId>(i), OrderType::GoodTillCancel, Side::Buy, 50, 10});
    }
    for (int i = 0; i < 1000000; i++){
        ob.addOrder(Order{static_cast<OrderId>(i), OrderType::GoodTillCancel, Side::Sell, 50, 100});
    }
    std::cout << timer.elapsed();
}
