#include <array>
#include <cstdint>
#include <cstring>
#include <iostream>

#include "order.h"
#include "protocol.h"

namespace {

void test_encode_decode_order() {
	Order original{};
	original.id = 12345;
	original.side = Side::Bid;
	original.action = OrderAction::Add;
	original.price = 150;
	original.quantity = 100;

	const auto wire = exchange::network::encode_wire_order(original);
	if (wire.magic != exchange::network::kOrderMagic) {
		std::cerr << "magic mismatch\n";
		throw std::runtime_error("magic mismatch");
	}

	std::array<std::byte, sizeof(wire)> buffer{};
	std::memcpy(buffer.data(), &wire, sizeof(wire));

	Order decoded{};
	if (!exchange::network::decode_wire_order(buffer.data(), buffer.size(), decoded)) {
		std::cerr << "decode failed\n";
		throw std::runtime_error("decode failed");
	}

	if (decoded.id != original.id || decoded.side != original.side ||
		decoded.action != original.action || decoded.price != original.price ||
		decoded.quantity != original.quantity) {
		std::cerr << "decoded order mismatch\n";
		throw std::runtime_error("decoded order mismatch");
	}

	std::cout << "test_encode_decode_order PASSED\n";
}

void test_encode_execution() {
	Execution exec{};
	exec.aggressorOrderId = 111;
	exec.restingOrderId = 222;
	exec.price = 100;
	exec.quantity = 50;

	const auto wire = exchange::network::encode_wire_execution(exec, Side::Bid);
	if (wire.magic != exchange::network::kExecutionMagic) {
		std::cerr << "execution magic mismatch\n";
		throw std::runtime_error("execution magic mismatch");
	}

	if (wire.aggressorOrderId != exec.aggressorOrderId ||
		wire.restingOrderId != exec.restingOrderId ||
		wire.price != exec.price || wire.quantity != exec.quantity) {
		std::cerr << "execution encoding mismatch\n";
		throw std::runtime_error("execution encoding mismatch");
	}

	std::cout << "test_encode_execution PASSED\n";
}

void test_multiple_order_sequence() {
	for (std::size_t i = 0; i < 1000; ++i) {
		Order order{};
		order.id = i + 1;
		order.side = (i % 2 == 0) ? Side::Bid : Side::Ask;
		order.action = OrderAction::Add;
		order.price = static_cast<Price>(100 + (i % 50));
		order.quantity = static_cast<Quantity>(1 + (i % 100));

		const auto wire = exchange::network::encode_wire_order(order);
		std::array<std::byte, sizeof(wire)> buffer{};
		std::memcpy(buffer.data(), &wire, sizeof(wire));

		Order decoded{};
		if (!exchange::network::decode_wire_order(buffer.data(), buffer.size(), decoded)) {
			std::cerr << "decode failed at sequence " << i << "\n";
			throw std::runtime_error("decode failed");
		}

		if (decoded.id != order.id || decoded.side != order.side || decoded.price != order.price) {
			std::cerr << "order mismatch at sequence " << i << "\n";
			throw std::runtime_error("order mismatch");
		}
	}

	std::cout << "test_multiple_order_sequence PASSED\n";
}

} // namespace

int main() {
	try {
		test_encode_decode_order();
		test_encode_execution();
		test_multiple_order_sequence();
		std::cout << "wire_protocol_test PASSED all tests\n";
		return 0;
	} catch (const std::exception& e) {
		std::cerr << "test failed: " << e.what() << "\n";
		return 1;
	}
}
