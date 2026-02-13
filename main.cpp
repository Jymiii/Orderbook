#include "synthetic_order_generator/OrderGenerator.h"
#include "synthetic_order_generator/OrderExecutor.h"

int main() {
    OrderExecutor executor{MarketState{}, 100000};
    executor.run("../data/orders.txt");
    return 0;
}
