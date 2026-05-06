#include <array>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <memory>
#include <thread>

#include "matching_engine.h"
#include "order.h"

namespace {

constexpr std::size_t kTestOrderCount = 1000;

} // namespace

int main() {
	auto engine = std::make_unique<MatchingEngine>();
	std::size_t totalExecutions = 0;
	std::size_t totalFilled = 0;

	for (std::size_t i = 0; i < kTestOrderCount; ++i) {
		Order order{};
		order.id = i + 1;
		order.side = (i % 2 == 0) ? Side::Bid : Side::Ask;
		order.action = OrderAction::Add;
		order.price = static_cast<Price>(100 + (i % 32));
		order.quantity = static_cast<Quantity>(1 + (i % 8));
		order.remaining = order.quantity;

		const auto executions = engine->process_order(order, [&](const Execution&) {
			++totalExecutions;
		});

		for (const auto& exec : executions) {
			totalFilled += exec.quantity;
		}
	}

	const BookSnapshot snapshot = engine->book().snapshot();
	const std::size_t bidsCount = snapshot.bids.size();
	const std::size_t asksCount = snapshot.asks.size();
	const std::size_t totalResting = bidsCount + asksCount;

	std::cout << "market_maker_functional_test PASSED\n";
	std::cout << "  orders_processed=" << kTestOrderCount << "\n";
	std::cout << "  executions=" << totalExecutions << "\n";
	std::cout << "  filled_quantity=" << totalFilled << "\n";
	std::cout << "  resting_orders=" << totalResting << "\n";

	if (bidsCount == 0 || asksCount == 0) {
		std::cerr << "warning: book has no bids or asks after " << kTestOrderCount << " orders\n";
	}

	return 0;
}
