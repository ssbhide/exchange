// Core integer types for the matching engine.

#pragma once

#include <cstddef>
#include <cstdint>
#include <limits>
#include <vector>

using Price = std::int32_t;
using Quantity = std::int32_t;
using OrderId = std::uint64_t;
using OrderIndex = std::size_t;

inline constexpr OrderIndex kInvalidOrderIndex = std::numeric_limits<OrderIndex>::max();

enum class Side {
	Bid,
	Ask,
};

enum class OrderAction {
	Add,
	Cancel,
	Modify,
};

struct Execution {
	OrderId aggressorOrderId{};
	OrderId restingOrderId{};
	Price price{};
	Quantity quantity{};
};

struct BookOrderSnapshot {
	OrderId id{};
	Side side{Side::Bid};
	Price price{};
	Quantity quantity{};
	Quantity remaining{};

	bool operator==(const BookOrderSnapshot&) const = default;
};

struct BookSnapshot {
	std::vector<BookOrderSnapshot> bids;
	std::vector<BookOrderSnapshot> asks;

	bool operator==(const BookSnapshot&) const = default;
};
