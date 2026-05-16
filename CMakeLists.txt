cmake_minimum_required(VERSION 3.25)
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

add_library(orderbook_lib
        src/orderbook/Orderbook.cpp
        src/orderbook/LevelData.h
)
target_include_directories(orderbook_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_compile_options(orderbook_lib PRIVATE -Wall -Wextra -Wpedantic)

option(ORDERBOOK_ENABLE_INSTRUMENTATION "Enable per-operation timing counters" OFF)
if (ORDERBOOK_ENABLE_INSTRUMENTATION)
    message(STATUS "Orderbook instrumentation: ON")
    target_compile_definitions(orderbook_lib PUBLIC ORDERBOOK_ENABLE_INSTRUMENTATION)
else()
    message(STATUS "Orderbook instrumentation: OFF")
endif()

add_executable(Orderbook
        main.cpp
        src/simulation/OrderGenerator.cpp
        src/simulation/OrderExecutor.cpp
)
target_compile_options(Orderbook PRIVATE -Wall -Wextra -Wpedantic)
target_link_libraries(Orderbook PRIVATE orderbook_lib)

enable_testing()

add_executable(OrderbookTests
        tests/orderbook/GoodTillCancelTest.cpp
        tests/orderbook/OrderModifyTest.cpp
        tests/orderbook/MarketOrderTest.cpp
        tests/orderbook/FillOrKillTest.cpp
        tests/orderbook/FillAndKillTest.cpp
        tests/orderbook/GoodForDayTest.cpp
        tests/orderbook/AdditionalTests.cpp
)
target_compile_options(OrderbookTests PRIVATE -Wall -Wextra -Wpedantic)
target_link_libraries(OrderbookTests PRIVATE orderbook_lib GTest::gtest_main)

include(GoogleTest)
gtest_discover_tests(OrderbookTests)
