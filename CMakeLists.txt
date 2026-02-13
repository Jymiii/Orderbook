cmake_minimum_required(VERSION 3.25)  # 4.0 is fine if you have it, but 3.25 is more common
project(Orderbook)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/archive/52eb8108c5bdec04579160ae17225d66034bd723.zip
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# ---- Library: code under test ----
add_library(orderbook_lib
        src/orderbook/Orderbook.cpp
)
target_include_directories(orderbook_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)

# ---- Main executable (NO tests here) ----
add_executable(Orderbook main.cpp
        src/synthetic_order_generator/OrderExecutor.h
        src/synthetic_order_generator/OrderGenerator.cpp
        src/synthetic_order_generator/OrderGenerator.h
        src/synthetic_order_generator/MarketState.h
        src/synthetic_order_generator/OrderEvent.h
        src/synthetic_order_generator/OrderRegistry.h
        src/synthetic_order_generator/Timer.h
        src/synthetic_order_generator/OrderExecutor.cpp)
target_link_libraries(Orderbook PRIVATE orderbook_lib)

# ---- Tests ----
enable_testing()

add_executable(OrderbookTests
        tests/orderbook/GoodTillCancelTest.cpp
        tests/orderbook/OrderModifyTest.cpp
        tests/orderbook/MarketOrderTest.cpp
        tests/orderbook/TestHelpers.h
        tests/orderbook/FillOrKillTest.cpp
        tests/orderbook/FillAndKillTest.cpp
        tests/orderbook/GoodForDayTest.cpp
)
target_link_libraries(OrderbookTests PRIVATE orderbook_lib GTest::gtest_main)

include(GoogleTest)
gtest_discover_tests(OrderbookTests)
