#include <array>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>

#include "matching_engine.h"
#include "protocol.h"
#include "udp_multicast.h"

namespace {

constexpr std::size_t kBufferSize = 4096;

} // namespace

int main(int argc, char** argv) {
	const std::string multicastAddress = argc > 1 ? argv[1] : "239.0.0.1";
	const std::uint16_t port = argc > 2 ? static_cast<std::uint16_t>(std::strtoul(argv[2], nullptr, 10)) : 5000;
	const std::string interfaceAddress = argc > 3 ? argv[3] : std::string{};

	exchange::network::UdpMulticastReceiver receiver(multicastAddress, port, interfaceAddress);
	if (!receiver.valid()) {
		std::cerr << "failed to join multicast feed\n";
		return 1;
	}

	auto engine = std::make_unique<MatchingEngine>();
	std::array<std::byte, kBufferSize> buffer{};
	std::size_t processedOrders = 0;

	while (true) {
		const ssize_t bytesRead = receiver.receive(buffer.data(), buffer.size());
		if (bytesRead <= 0) {
			continue;
		}

		Order order{};
		if (!exchange::network::decode_wire_order(buffer.data(), static_cast<std::size_t>(bytesRead), order)) {
			continue;
		}

		engine->process_order(order, [](const Execution&) {});
		++processedOrders;
		if (processedOrders % 1000 == 0) {
			std::cout << "processed_orders=" << processedOrders << '\n';
		}
	}

	return 0;
}