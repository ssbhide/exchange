#pragma once

#include <array>
#include <cstddef>
#include <utility>

#include "types.h"

template <typename T, std::size_t Size>
class MemoryPool {
public:
    using index_type = OrderIndex;

    MemoryPool() {
        reset();
    }

    template <typename... Args>
    index_type allocate(Args&&... args) {
        if (free_head_ == kInvalidOrderIndex) {
            return kInvalidOrderIndex;
        }

        const index_type index = free_head_;
        free_head_ = free_next_[index];
        used_[index] = true;
        storage_[index] = T{std::forward<Args>(args)...};
        return index;
    }

    void release(index_type index) {
        if (!valid(index) || !used_[index]) {
            return;
        }

        used_[index] = false;
        storage_[index] = T{};
        free_next_[index] = free_head_;
        free_head_ = index;
    }

    T& get(index_type index) {
        return storage_[index];
    }

    const T& get(index_type index) const {
        return storage_[index];
    }

    bool contains(index_type index) const {
        return valid(index) && used_[index];
    }

    void reset() {
        for (index_type i = 0; i < Size; ++i) {
            free_next_[i] = i + 1;
            used_[i] = false;
            storage_[i] = T{};
        }

        if constexpr (Size > 0) {
            free_next_[Size - 1] = kInvalidOrderIndex;
            free_head_ = 0;
        } else {
            free_head_ = kInvalidOrderIndex;
        }
    }

    static constexpr std::size_t capacity() {
        return Size;
    }

private:
    static constexpr bool valid(index_type index) {
        return index < Size;
    }

    std::array<T, Size> storage_{};
    std::array<index_type, Size> free_next_{};
    std::array<bool, Size> used_{};
    index_type free_head_{kInvalidOrderIndex};
};