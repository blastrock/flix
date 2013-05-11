cmake_minimum_required(VERSION 2.8)
project(flix)

set(LIBKCXX_INCLUDE_DIRS ${CMAKE_SOURCE_DIR}/libcxx/include
                         ${CMAKE_SOURCE_DIR}/libkc/include
                         /home/blastrock/.usr/lib/clang/3.2/include)

set(LIBCXX_ENABLE_EXCEPTIONS OFF CACHE BOOL "Use exceptions.")
set(LIBCXX_ENABLE_RTTI OFF CACHE BOOL "Use run time type information.")
set(LIBCXX_ENABLE_ASSERTIONS ON CACHE BOOL "Enable assertions independent of build mode.")
set(LIBCXX_ENABLE_PEDANTIC ON CACHE BOOL "Compile with pedantic enabled.")
set(LIBCXX_ENABLE_WERROR OFF CACHE BOOL "Fail and stop if a warning is triggered.")
set(LIBCXX_ENABLE_CXX0X ON CACHE BOOL "Enable -std=c++0x and use of c++0x language features if the compiler supports it.")
set(LIBCXX_ENABLE_SHARED OFF CACHE BOOL "Build libc++ as a shared library.")

add_subdirectory(libkcxx)
add_subdirectory(source)
