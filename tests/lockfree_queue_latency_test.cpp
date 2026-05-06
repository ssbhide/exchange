#include <atomic>
#include <cstdint>
#include <chrono>
#include <iostream>
#include <thread>
#include <vector>

#include "lockfree_queue.h"
#include "order.h"

#if defined(__x86_64__) || defined(__i386__)
#include <x86intrin.h>
static inline std::uint64_t read_tsc() {
    return __rdtsc();
}
#elif defined(__APPLE__)
#include <mach/mach_time.h>
static inline std::uint64_t read_tsc() {
    return mach_absolute_time();
}
#else
static inline std::uint64_t read_tsc() {
    return static_cast<std::uint64_t>(std::chrono::steady_clock::now().time_since_epoch().count());
}
#endif

namespace {

constexpr std::size_t kMessageCount = 1'000'000;
constexpr std::size_t kQueueCapacity = 1 << 16;

} // namespace

int main() {
    LockFreeQueue<Order, kQueueCapacity> queue;
    std::vector<std::uint64_t> sendTimestamps(kMessageCount, 0);
    std::atomic<bool> start{false};

    std::uint64_t totalLatency = 0;

    std::thread producer([&] {
        while (!start.load(std::memory_order_acquire)) {
        }

        for (std::size_t i = 0; i < kMessageCount; ++i) {
            Order order{};
            order.id = static_cast<OrderId>(i);
            order.side = (i % 2 == 0) ? Side::Bid : Side::Ask;
            order.action = OrderAction::Add;
            order.price = static_cast<Price>(100 + (i % 32));
            order.quantity = 1;
            order.remaining = 1;

            const std::uint64_t tsc = read_tsc();
            sendTimestamps[i] = tsc;

            while (!queue.push(order)) {
            }
        }
    });

    std::thread consumer([&] {
        while (!start.load(std::memory_order_acquire)) {
        }

        Order order{};
        for (std::size_t i = 0; i < kMessageCount; ++i) {
            while (!queue.pop(order)) {
            }

            const std::uint64_t receiveTsc = read_tsc();
            totalLatency += receiveTsc - sendTimestamps[static_cast<std::size_t>(order.id)];
        }
    });

    start.store(true, std::memory_order_release);

    producer.join();
    consumer.join();

    const double averageCycles = static_cast<double>(totalLatency) / static_cast<double>(kMessageCount);
    std::cout << "average_cycles_per_message=" << averageCycles << '\n';
    return 0;
}