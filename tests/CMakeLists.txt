add_executable(lockfree_queue_latency_test
    lockfree_queue_latency_test.cpp
)

target_link_libraries(lockfree_queue_latency_test PRIVATE
    exchange_core
    exchange_utils
    Threads::Threads
)

add_test(NAME lockfree_queue_latency_test COMMAND lockfree_queue_latency_test)

add_executable(passive_engine_replication_test
    passive_engine_replication_test.cpp
)

target_link_libraries(passive_engine_replication_test PRIVATE
    exchange_core
    exchange_network
    Threads::Threads
)

add_test(NAME passive_engine_replication_test COMMAND passive_engine_replication_test)

add_executable(wire_protocol_test
	wire_protocol_test.cpp
)

target_link_libraries(wire_protocol_test PRIVATE
	exchange_core
	exchange_network
	Threads::Threads
)

add_test(NAME wire_protocol_test COMMAND wire_protocol_test)

add_executable(market_maker_functional_test
	market_maker_functional_test.cpp
)

target_link_libraries(market_maker_functional_test PRIVATE
	exchange_core
	exchange_network
	Threads::Threads
)

add_test(NAME market_maker_functional_test COMMAND market_maker_functional_test)

add_executable(order_book_stress_test
	order_book_stress_test.cpp
)

target_link_libraries(order_book_stress_test PRIVATE
	exchange_core
	Threads::Threads
)

add_test(NAME order_book_stress_test COMMAND order_book_stress_test)

add_executable(cancel_replace_test
	cancel_replace_test.cpp
)

target_link_libraries(cancel_replace_test PRIVATE
	exchange_core
	Threads::Threads
)

add_test(NAME cancel_replace_test COMMAND cancel_replace_test)