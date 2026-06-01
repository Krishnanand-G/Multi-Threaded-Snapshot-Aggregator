cmake_minimum_required(VERSION 3.14)
project(SnapshotAggregator VERSION 1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")

add_executable(SnapshotAggregator src/main.cpp)
