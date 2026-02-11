cmake_minimum_required(VERSION 4.0)
project(Orderbook)

set(CMAKE_CXX_STANDARD 23)

add_executable(Orderbook main.cpp
        src/OrderType.h
        src/Side.h
        src/LevelInfo.h
        src/OrderbookLevelInfos.h
        src/Order.h
        src/OrderModify.h
        src/Trade.h
        src/Orderbook.cpp
        src/Orderbook.h
        src/Usings.h
        src/Constants.h)
