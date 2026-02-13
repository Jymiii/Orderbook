#include "synthetic_order_generator/OrderGenerator.h"
#include "synthetic_order_generator/OrderExecutor.h"

int main() {
    OrderExecutor executor{MarketState{}, 1000000};
    executor.run();
    return 0;
}
