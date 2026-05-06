#pragma once

#include <array>
#include <atomic>
#include <cstddef>

template <typename T, std::size_t Size>
class LockFreeQueue {
public:
    static_assert(Size > 1, "LockFreeQueue requires capacity greater than 1");

    bool push(const T& value) {
        const std::size_t currentTail = tail_.load(std::memory_order_relaxed);
        const std::size_t nextTail = increment(currentTail);

        if (nextTail == head_.load(std::memory_order_acquire)) {
            return false;
        }

        buffer_[currentTail] = value;
        tail_.store(nextTail, std::memory_order_release);
        return true;
    }

    bool pop(T& value) {
        const std::size_t currentHead = head_.load(std::memory_order_relaxed);

        if (currentHead == tail_.load(std::memory_order_acquire)) {
            return false;
        }

        value = buffer_[currentHead];
        head_.store(increment(currentHead), std::memory_order_release);
        return true;
    }

    bool empty() const {
        return head_.load(std::memory_order_acquire) == tail_.load(std::memory_order_acquire);
    }

    constexpr std::size_t capacity() const {
        return Size - 1;
    }

private:
    static constexpr std::size_t increment(std::size_t index) {
        return (index + 1) % Size;
    }

    std::array<T, Size> buffer_{};
    std::atomic<std::size_t> head_{0};
    std::atomic<std::size_t> tail_{0};
};