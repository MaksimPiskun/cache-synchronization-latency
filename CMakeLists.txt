cmake_minimum_required(VERSION 3.10)
project(cache_sync_latency)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

add_executable(bench bench.cpp)
