#include "synthetic_order_generator/OrderExecutor.h"

int main() {
    OrderExecutor ex = {MarketState{}, 10000};
    std::cout << "Took " << ex.run("../data/orders.txt") << "\n\n";
}