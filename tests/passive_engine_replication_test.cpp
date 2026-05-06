#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <memory>
#include <random>
#include <vector>

#include "matching_engine.h"
#include "protocol.h"

namespace {

constexpr std::size_t kOrderCount = 100000;

Order make_order(std::uint64_t sequence, std::mt19937_64& rng) {
	Order order{};
	order.id = sequence + 1;
	order.side = (sequence % 2 == 0) ? Side::Bid : Side::Ask;
	order.action = OrderAction::Add;
	order.price = static_cast<Price>(100 + (rng() % 32));
	order.quantity = static_cast<Quantity>(1 + (rng() % 8));
	order.remaining = order.quantity;
	return order;
}

} // namespace

int main() {
	auto primaryEngine = std::make_unique<MatchingEngine>();
	auto passiveEngine = std::make_unique<MatchingEngine>();
	std::mt19937_64 rng(0xC0FFEE);

	for (std::size_t i = 0; i < kOrderCount; ++i) {
		const Order order = make_order(i, rng);
		const auto wire = exchange::network::encode_wire_order(order);
		std::array<std::byte, sizeof(wire)> bytes{};
		std::memcpy(bytes.data(), &wire, sizeof(wire));

		Order decodedOrder{};
		if (!exchange::network::decode_wire_order(bytes.data(), bytes.size(), decodedOrder)) {
			std::cerr << "failed to decode order " << i << '\n';
			return 1;
		}

		primaryEngine->process_order(order, [](const Execution&) {});
		passiveEngine->process_order(decodedOrder, [](const Execution&) {});
	}

	if (primaryEngine->book().snapshot() != passiveEngine->book().snapshot()) {
		std::cerr << "book snapshots diverged after " << kOrderCount << " orders\n";
		return 1;
	}

	std::cout << "replication_verified orders=" << kOrderCount << '\n';
	return 0;
}