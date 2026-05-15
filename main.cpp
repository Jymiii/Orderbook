#include "synthetic_order_generator/MarketState.h"
#include "synthetic_order_generator/OrderExecutor.h"

#include <iostream>

int main() {
    OrderExecutor ex{MarketState{}, 10000};
    std::cout << "Took " << ex.run() << "\n\n";
}