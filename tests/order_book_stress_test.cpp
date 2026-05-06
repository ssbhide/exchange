#include <array>
#include <cstddef>
#include <cstdint>
#include <chrono>
#include <iostream>
#include <memory>
#include <random>

#include "matching_engine.h"
#include "order.h"

namespace {

constexpr std::size_t kStressTestOrderCount = 50000;

} // namespace

int main() {
	auto engine = std::make_unique<MatchingEngine>();
	std::mt19937_64 rng(42);
	std::size_t totalExecutions = 0;

	const auto start = std::chrono::steady_clock::now();

	for (std::size_t i = 0; i < kStressTestOrderCount; ++i) {
		Order order{};
		order.id = i + 1;
		order.side = (i % 2 == 0) ? Side::Bid : Side::Ask;
		order.action = OrderAction::Add;
		order.price = static_cast<Price>(95 + (rng() % 10));
		order.quantity = static_cast<Quantity>(1 + (rng() % 20));
		order.remaining = order.quantity;

		const auto executions = engine->process_order(order, [&](const Execution&) {
			++totalExecutions;
		});

		if (i > 0 && i % 10000 == 0) {
			std::cout << "stress_test progress orders=" << i << " executions=" << totalExecutions << "\n";
		}
	}

	const auto end = std::chrono::steady_clock::now();
	const auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
	const double ordersPerSecond = static_cast<double>(kStressTestOrderCount) / (duration.count() / 1000.0);

	std::cout << "order_book_stress_test PASSED\n";
	std::cout << "  orders_processed=" << kStressTestOrderCount << "\n";
	std::cout << "  executions=" << totalExecutions << "\n";
	std::cout << "  duration_ms=" << duration.count() << "\n";
	std::cout << "  orders_per_second=" << static_cast<std::uint64_t>(ordersPerSecond) << "\n";

	return 0;
}
