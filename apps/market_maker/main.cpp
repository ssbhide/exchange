#include <arpa/inet.h>
#include <array>
#include <atomic>
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <random>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>

#include "order.h"
#include "protocol.h"

namespace {

constexpr std::uint16_t kDefaultPort = 5000;
constexpr std::size_t kMaxOrdersPerSecond = 1000;
constexpr Price kMidPrice = 100;
constexpr Price kSpread = 2;

} // namespace

class MarketMakerBot {
public:
	MarketMakerBot(const std::string& host, std::uint16_t port)
		: host_(host), port_(port) {}

	~MarketMakerBot() {
		disconnect();
	}

	bool connect() {
		socketFd_ = ::socket(AF_INET, SOCK_STREAM, 0);
		if (socketFd_ < 0) {
			std::cerr << "failed to create socket\n";
			return false;
		}

		sockaddr_in address{};
		address.sin_family = AF_INET;
		address.sin_port = htons(port_);
		if (::inet_pton(AF_INET, host_.c_str(), &address.sin_addr) <= 0) {
			std::cerr << "invalid host address\n";
			return false;
		}

		if (::connect(socketFd_, reinterpret_cast<sockaddr*>(&address), sizeof(address)) < 0) {
			std::cerr << "failed to connect to " << host_ << ":" << port_ << "\n";
			return false;
		}

		return true;
	}

	void disconnect() {
		if (socketFd_ >= 0) {
			::close(socketFd_);
			socketFd_ = -1;
		}
	}

	bool send_order(const Order& order) {
		if (socketFd_ < 0) {
			return false;
		}

		const auto wire = exchange::network::encode_wire_order(order);
		const ssize_t sent = ::send(socketFd_, &wire, sizeof(wire), 0);
		return sent == static_cast<ssize_t>(sizeof(wire));
	}

	void run_for_duration(std::chrono::seconds duration) {
		std::mt19937_64 rng(std::random_device{}());
		std::uniform_int_distribution<std::size_t> spreadDist(1, kSpread);
		std::uniform_int_distribution<Quantity> quantityDist(1, 5);

		const auto start = std::chrono::steady_clock::now();
		const auto end = start + duration;
		std::size_t orderCount = 0;
		Price midPrice = kMidPrice;

		while (std::chrono::steady_clock::now() < end) {
			const Price bidPrice = midPrice - static_cast<Price>(spreadDist(rng));
			const Price askPrice = midPrice + static_cast<Price>(spreadDist(rng));
			const Quantity qty = quantityDist(rng);

			Order bidOrder{};
			bidOrder.id = ++nextOrderId_;
			bidOrder.side = Side::Bid;
			bidOrder.action = OrderAction::Add;
			bidOrder.price = bidPrice;
			bidOrder.quantity = qty;

			Order askOrder{};
			askOrder.id = ++nextOrderId_;
			askOrder.side = Side::Ask;
			askOrder.action = OrderAction::Add;
			askOrder.price = askPrice;
			askOrder.quantity = qty;

			if (send_order(bidOrder) && send_order(askOrder)) {
				orderCount += 2;
				++midPrice;
				if (midPrice > kMidPrice + 5) {
					midPrice = kMidPrice - 5;
				}
			}

			if (orderCount % 100 == 0) {
				std::cout << "orders_sent=" << orderCount << '\n';
			}

			std::this_thread::sleep_for(std::chrono::milliseconds(1000 / kMaxOrdersPerSecond));
		}

		std::cout << "market_maker_completed orders=" << orderCount << '\n';
	}

private:
	int socketFd_{-1};
	std::string host_;
	std::uint16_t port_;
	std::uint64_t nextOrderId_{0};
};

int main(int argc, char** argv) {
	const std::string host = argc > 1 ? argv[1] : "127.0.0.1";
	const std::uint16_t port = argc > 2 ? static_cast<std::uint16_t>(std::strtoul(argv[2], nullptr, 10)) : kDefaultPort;

	MarketMakerBot bot(host, port);
	if (!bot.connect()) {
		std::cerr << "market maker connection failed\n";
		return 1;
	}

	std::cout << "market_maker connected to " << host << ":" << port << '\n';
	bot.run_for_duration(std::chrono::seconds(5));

	return 0;
}
