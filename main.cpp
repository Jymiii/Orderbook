#include "synthetic_order_generator/OrderExecutor.h"

int main() {
    OrderExecutor ex = {MarketState{}, 40000};
    std::cout << "Took " << ex.run("../data/orders.txt");
}