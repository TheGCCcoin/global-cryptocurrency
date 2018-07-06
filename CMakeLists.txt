cmake_minimum_required(VERSION 2.8.4)
project(thegcc-dash)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

add_custom_target(thegcc-b COMMAND make -C ${thegcc-dash_SOURCE_DIR}
        CLION_EXE_DIR=${PROJECT_BINARY_DIR})

add_custom_target(thegcc-qt-b COMMAND make -C ${thegcc-dash_SOURCE_DIR}/src/qt
        CLION_EXE_DIR=${PROJECT_BINARY_DIR} bitcoin-qt)

add_custom_target(bitcoind COMMAND make -C ${thegcc-dash_SOURCE_DIR}/src
        CLION_EXE_DIR=${PROJECT_BINARY_DIR} bitcoind)
