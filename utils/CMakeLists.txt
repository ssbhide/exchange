add_library(exchange_utils INTERFACE)

target_include_directories(exchange_utils INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(exchange_utils INTERFACE exchange_core)