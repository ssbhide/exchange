add_executable(market_maker
	main.cpp
)

target_link_libraries(market_maker PRIVATE
	exchange_core
	exchange_network
	Threads::Threads
)
