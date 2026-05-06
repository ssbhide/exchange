cmake_minimum_required(VERSION 3.20)

project(exchange LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Threads REQUIRED)

enable_testing()

add_subdirectory(core)
add_subdirectory(network)
add_subdirectory(apps)
add_subdirectory(utils)
add_subdirectory(tests)
