cmake_minimum_required(VERSION 2.8.4)
#include(PrecompiledHeader.cmake)
project(thegcc)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

add_custom_target(thegcc-b COMMAND make -C ${thegcc_SOURCE_DIR}/src
        CLION_EXE_DIR=${PROJECT_BINARY_DIR} -j 1 TheGCCcoind)
#add_custom_target(thegcc-b COMMAND make -C ${thegcc_SOURCE_DIR}/src
#        CLION_EXE_DIR=${PROJECT_BINARY_DIR} -j 2 TheGCCcoind  ${thegcc_SOURCE_DIR}/src/thegcc/imports.cpp ${thegcc_SOURCE_DIR}/src/thegcc/imports.h)

#add_precompiled_header(thegcc-b ${thegcc_SOURCE_DIR}/src/thegcc/imports.h SOURCE_CXX ${thegcc_SOURCE_DIR}/src/thegcc/imports.cpp FORCEINCLUDE)

add_custom_target(thegcc-qt-b COMMAND make -C ${thegcc_SOURCE_DIR}/src/qt
        CLION_EXE_DIR=${PROJECT_BINARY_DIR} -j 2 TheGCCcoin-qt)

add_custom_target(bitcoind COMMAND make -C ${thegcc_SOURCE_DIR}/src
        CLION_EXE_DIR=${PROJECT_BINARY_DIR} bitcoind)
