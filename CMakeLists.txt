cmake_minimum_required(VERSION 3.16)

project(coroworker)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_VERBOSE_MAKEFILE ON)

find_package(Threads REQUIRED)


add_executable(coroworker coroworker.cpp)
target_link_libraries(coroworker PRIVATE Threads::Threads)
target_include_directories(coroworker PRIVATE common)
