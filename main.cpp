#include "simulation/MarketState.h"
#include "simulation/OrderExecutor.h"

#include <iostream>

int main() {
    OrderExecutor ex{MarketState{}, 10000};
    std::cout << "Took " << ex.run() << "\n\n";
}