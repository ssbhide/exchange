#include <cstddef>
#include <iostream>
#include <memory>

#include "matching_engine.h"
#include "order.h"

namespace {

constexpr std::size_t kCancelReplaceSequences = 100;

} // namespace

int main() {
	auto engine = std::make_unique<MatchingEngine>();
	std::size_t totalOperations = 0;

	for (std::size_t seq = 0; seq < kCancelReplaceSequences; ++seq) {
		const OrderId orderId = seq + 1;

		Order add{};
		add.id = orderId;
		add.side = Side::Bid;
		add.action = OrderAction::Add;
		add.price = 100 + static_cast<Price>(seq % 5);
		add.quantity = 10;
		add.remaining = 10;
		engine->process_order(add, [](const Execution&) {});
		++totalOperations;

		for (std::size_t i = 0; i < 5; ++i) {
			Order modify{};
			modify.id = orderId;
			modify.side = Side::Bid;
			modify.action = OrderAction::Modify;
			modify.price = 100 + static_cast<Price>((seq + i) % 10);
			modify.quantity = 10 - static_cast<Quantity>(i);
			modify.remaining = modify.quantity;
			engine->process_order(modify, [](const Execution&) {});
			++totalOperations;
		}

		Order cancel{};
		cancel.id = orderId;
		cancel.side = Side::Bid;
		cancel.action = OrderAction::Cancel;
		cancel.price = 100;
		cancel.quantity = 0;
		engine->process_order(cancel, [](const Execution&) {});
		++totalOperations;
	}

	const BookSnapshot snapshot = engine->book().snapshot();
	if (!snapshot.bids.empty() || !snapshot.asks.empty()) {
		std::cerr << "book is not empty after all cancels\n";
		return 1;
	}

	std::cout << "cancel_replace_test PASSED\n";
	std::cout << "  sequences=" << kCancelReplaceSequences << "\n";
	std::cout << "  total_operations=" << totalOperations << "\n";

	return 0;
}
