#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>

#include "order.h"

namespace exchange::network {

inline constexpr std::uint32_t kOrderMagic = 0x584f5244; // "XORD"
inline constexpr std::uint16_t kOrderWireVersion = 1;
inline constexpr std::uint32_t kExecutionMagic = 0x58455843; // "XEXC"
inline constexpr std::uint16_t kExecutionWireVersion = 1;

#pragma pack(push, 1)
struct WireOrderMessage {
    std::uint32_t magic{0};
    std::uint16_t version{0};
    std::uint8_t action{0};
    std::uint8_t side{0};
    std::uint64_t orderId{0};
    std::int32_t price{0};
    std::int32_t quantity{0};
};

struct WireExecutionMessage {
    std::uint32_t magic{0};
    std::uint16_t version{0};
    std::uint8_t side{0};
    std::uint8_t reserved{0};
    std::uint64_t aggressorOrderId{0};
    std::uint64_t restingOrderId{0};
    std::int32_t price{0};
    std::int32_t quantity{0};
};
#pragma pack(pop)

inline bool decode_wire_order(const std::byte* data, std::size_t size, Order& order) {
    if (size < sizeof(WireOrderMessage)) {
        return false;
    }

    WireOrderMessage message{};
    std::memcpy(&message, data, sizeof(message));

    if (message.magic != kOrderMagic || message.version != kOrderWireVersion) {
        return false;
    }

    if (message.action > static_cast<std::uint8_t>(OrderAction::Modify) || message.side > static_cast<std::uint8_t>(Side::Ask)) {
        return false;
    }

    order.id = message.orderId;
    order.action = static_cast<OrderAction>(message.action);
    order.side = static_cast<Side>(message.side);
    order.price = static_cast<Price>(message.price);
    order.quantity = static_cast<Quantity>(message.quantity);
    order.remaining = order.quantity;
    order.prev = kInvalidOrderIndex;
    order.next = kInvalidOrderIndex;
    order.active = false;
    return true;
}

inline WireOrderMessage encode_wire_order(const Order& order) {
    return WireOrderMessage{
        .magic = kOrderMagic,
        .version = kOrderWireVersion,
        .action = static_cast<std::uint8_t>(order.action),
        .side = static_cast<std::uint8_t>(order.side),
        .orderId = order.id,
        .price = static_cast<std::int32_t>(order.price),
        .quantity = static_cast<std::int32_t>(order.quantity),
    };
}

inline WireExecutionMessage encode_wire_execution(const Execution& execution, Side side) {
    return WireExecutionMessage{
        .magic = kExecutionMagic,
        .version = kExecutionWireVersion,
        .side = static_cast<std::uint8_t>(side),
        .reserved = 0,
        .aggressorOrderId = execution.aggressorOrderId,
        .restingOrderId = execution.restingOrderId,
        .price = static_cast<std::int32_t>(execution.price),
        .quantity = static_cast<std::int32_t>(execution.quantity),
    };
}

} // namespace exchange::network