#include "synthetic_order_generator/OrderExecutor.h"

int main() {
    double tot = 0;
    int count = 50;
    for (int i = 0; i < count; i++) {
        OrderExecutor ex{MarketState{}, 100000};
        double time = ex.run("orders.txt");
        tot += time;
    }
    std::cout << "Average: " << tot / count << " nano seconds per order.";
}