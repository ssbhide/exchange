add_executable(passive_engine
	main.cpp
)

target_link_libraries(passive_engine PRIVATE
	exchange_core
	exchange_network
	Threads::Threads
)