cmake_minimum_required(VERSION 3.25)
project(Orderbook)

if(CMAKE_BUILD_TYPE STREQUAL "Release")
    add_compile_options(-march=native)
    include(CheckIPOSupported)
    check_ipo_supported(RESULT ipo_ok OUTPUT ipo_err)
    if(ipo_ok)
        set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
    else()
        message(STATUS "IPO/LTO not supported by toolchain: ${ipo_err}")
    endif()
endif()

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/archive/52eb8108c5bdec04579160ae17225d66034bd723.zip
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

FetchContent_Declare(
        Boost
        URL https://github.com/boostorg/boost/releases/download/boost-1.86.0/boost-1.86.0-cmake.tar.xz
        URL_HASH SHA256=2c5ec5edcdff47ff55e27ed9560b0a0b94b07bd07ed9928b476150e16b0efc57
)
set(BOOST_INCLUDE_LIBRARIES unordered)
set(BOOST_ENABLE_CMAKE ON)
FetchContent_MakeAvailable(Boost)

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
        tests/GoodTillCancelTest.cpp
        tests/OrderModifyTest.cpp
        tests/MarketOrderTest.cpp
        tests/FillOrKillTest.cpp
        tests/FillAndKillTest.cpp
        tests/GoodForDayTest.cpp
        tests/AdditionalTests.cpp
)
target_compile_options(OrderbookTests PRIVATE -Wall -Wextra -Wpedantic)
target_link_libraries(OrderbookTests PRIVATE orderbook_lib GTest::gtest_main)
target_link_libraries(orderbook_lib PUBLIC Boost::unordered)

include(GoogleTest)
gtest_discover_tests(OrderbookTests)
