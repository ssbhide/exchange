add_library(exchange_network INTERFACE)

target_include_directories(exchange_network INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(exchange_network INTERFACE
    exchange_core
    exchange_utils
    Threads::Threads
)