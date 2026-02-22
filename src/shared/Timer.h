//
// Created by Jimi van der Meer on 12/02/2026.
//

#ifndef ORDERBOOK_TIMER_H
#define ORDERBOOK_TIMER_H

#include <chrono>

class Timer {
private:
    using Clock = std::chrono::steady_clock;
    using Second = std::chrono::duration<double, std::ratio<1> >;

    std::chrono::time_point<Clock> m_beg{Clock::now()};

public:
    void reset() {
        m_beg = Clock::now();
    }

    void start() {
        m_beg = Clock::now();
    }

    [[nodiscard]] double elapsed() const {
        return std::chrono::duration_cast<Second>(Clock::now() - m_beg).count();
    }
};

#endif //ORDERBOOK_TIMER_H