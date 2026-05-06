#pragma once

#include <algorithm>
#include <cstddef>
#include <functional>
#include <map>
#include <unordered_map>
#include <utility>
#include <vector>

#include "memory_pool.h"
#include "order.h"

template <std::size_t Capacity = 65536>
class OrderBook {
public:
	struct PriceLevel {
		OrderIndex head{kInvalidOrderIndex};
		OrderIndex tail{kInvalidOrderIndex};
	};

	bool add_order(const Order& order) {
		if (order.id == 0 || order.quantity <= 0 || order.price <= 0) {
			return false;
		}

		if (orderIdToIndex_.find(order.id) != orderIdToIndex_.end()) {
			return false;
		}

		const OrderIndex index = allocate_order(order);
		if (index == kInvalidOrderIndex) {
			return false;
		}

		attach_order(index);
		return true;
	}

	bool cancel_order(OrderId orderId) {
		auto orderIt = orderIdToIndex_.find(orderId);
		if (orderIt == orderIdToIndex_.end()) {
			return false;
		}

		remove_order(orderIt->second);
		return true;
	}

	bool modify_order(OrderId orderId, const Order& updatedOrder) {
		auto orderIt = orderIdToIndex_.find(orderId);
		if (orderIt == orderIdToIndex_.end()) {
			return false;
		}

		const Order storedOrder = orders_.get(orderIt->second);
		remove_order(orderIt->second);

		Order replacement = updatedOrder;
		replacement.id = orderId;
		replacement.side = storedOrder.side;

		return add_order(replacement);
	}

	OrderIndex best_bid_index() const {
		if (bids_.empty()) {
			return kInvalidOrderIndex;
		}

		return bids_.begin()->second.head;
	}

	OrderIndex best_ask_index() const {
		if (asks_.empty()) {
			return kInvalidOrderIndex;
		}

		return asks_.begin()->second.head;
	}

	const Order* get_order(OrderId orderId) const {
		auto it = orderIdToIndex_.find(orderId);
		if (it == orderIdToIndex_.end()) {
			return nullptr;
		}

		return &orders_.get(it->second);
	}

	Order* get_order(OrderId orderId) {
		auto it = orderIdToIndex_.find(orderId);
		if (it == orderIdToIndex_.end()) {
			return nullptr;
		}

		return &orders_.get(it->second);
	}

	template <typename ExecutionHandler>
	Quantity match(Order& incoming, ExecutionHandler&& executionHandler) {
		Quantity filled = 0;

		if (incoming.side == Side::Bid) {
			while (incoming.quantity > 0 && !asks_.empty()) {
				auto bestLevelIt = asks_.begin();
				const Price bestPrice = bestLevelIt->first;
				if (incoming.price < bestPrice) {
					break;
				}

				OrderIndex restingIndex = bestLevelIt->second.head;
				while (incoming.quantity > 0 && restingIndex != kInvalidOrderIndex) {
					Order& restingOrder = orders_.get(restingIndex);
					const Quantity tradeQuantity = std::min(incoming.quantity, restingOrder.quantity);
					incoming.quantity -= tradeQuantity;
					restingOrder.quantity -= tradeQuantity;
					filled += tradeQuantity;

					executionHandler(Execution{
						.aggressorOrderId = incoming.id,
						.restingOrderId = restingOrder.id,
						.price = bestPrice,
						.quantity = tradeQuantity,
					});

					const OrderIndex nextIndex = restingOrder.next;
					if (restingOrder.quantity == 0) {
						remove_order(restingIndex);
					}
					restingIndex = nextIndex;
				}
			}
		} else {
			while (incoming.quantity > 0 && !bids_.empty()) {
				auto bestLevelIt = bids_.begin();
				const Price bestPrice = bestLevelIt->first;
				if (incoming.price > bestPrice) {
					break;
				}

				OrderIndex restingIndex = bestLevelIt->second.head;
				while (incoming.quantity > 0 && restingIndex != kInvalidOrderIndex) {
					Order& restingOrder = orders_.get(restingIndex);
					const Quantity tradeQuantity = std::min(incoming.quantity, restingOrder.quantity);
					incoming.quantity -= tradeQuantity;
					restingOrder.quantity -= tradeQuantity;
					filled += tradeQuantity;

					executionHandler(Execution{
						.aggressorOrderId = incoming.id,
						.restingOrderId = restingOrder.id,
						.price = bestPrice,
						.quantity = tradeQuantity,
					});

					const OrderIndex nextIndex = restingOrder.next;
					if (restingOrder.quantity == 0) {
						remove_order(restingIndex);
					}
					restingIndex = nextIndex;
				}
			}
		}

		return filled;
	}

	Quantity resting_quantity(OrderId orderId) const {
		const Order* order = get_order(orderId);
		return order == nullptr ? 0 : order->quantity;
	}

	BookSnapshot snapshot() const {
		BookSnapshot state;
		for (const auto& [price, level] : bids_) {
			append_level_snapshot(state.bids, level, Side::Bid, price);
		}

		for (const auto& [price, level] : asks_) {
			append_level_snapshot(state.asks, level, Side::Ask, price);
		}

		return state;
	}

private:
	void append_level_snapshot(std::vector<BookOrderSnapshot>& out, const PriceLevel& level, Side side, Price price) const {
		OrderIndex current = level.head;
		while (current != kInvalidOrderIndex) {
			const Order& order = orders_.get(current);
			out.push_back(BookOrderSnapshot{
				.id = order.id,
				.side = side,
				.price = price,
				.quantity = order.quantity,
				.remaining = order.remaining,
			});
			current = order.next;
		}
	}

	OrderIndex allocate_order(const Order& order) {
		const OrderIndex index = orders_.allocate(order);
		if (index == kInvalidOrderIndex) {
			return kInvalidOrderIndex;
		}

		Order& storedOrder = orders_.get(index);
		storedOrder.remaining = storedOrder.quantity;
		storedOrder.prev = kInvalidOrderIndex;
		storedOrder.next = kInvalidOrderIndex;
		storedOrder.active = true;
		orderIdToIndex_[storedOrder.id] = index;
		return index;
	}

	void attach_order(OrderIndex index) {
		Order& order = orders_.get(index);
		if (order.side == Side::Bid) {
			PriceLevel& level = bids_[order.price];

			if (level.tail == kInvalidOrderIndex) {
				level.head = index;
				level.tail = index;
			} else {
				Order& tail = orders_.get(level.tail);
				tail.next = index;
				order.prev = level.tail;
				level.tail = index;
			}
		} else {
			PriceLevel& level = asks_[order.price];

			if (level.tail == kInvalidOrderIndex) {
				level.head = index;
				level.tail = index;
			} else {
				Order& tail = orders_.get(level.tail);
				tail.next = index;
				order.prev = level.tail;
				level.tail = index;
			}
		}
	}

	void detach_from_level(OrderIndex index) {
		Order& order = orders_.get(index);
		if (order.side == Side::Bid) {
			auto levelIt = bids_.find(order.price);
			if (levelIt == bids_.end()) {
				return;
			}

			PriceLevel& level = levelIt->second;
			if (order.prev != kInvalidOrderIndex) {
				orders_.get(order.prev).next = order.next;
			} else {
				level.head = order.next;
			}

			if (order.next != kInvalidOrderIndex) {
				orders_.get(order.next).prev = order.prev;
			} else {
				level.tail = order.prev;
			}

			if (level.head == kInvalidOrderIndex) {
				bids_.erase(levelIt);
			}
		} else {
			auto levelIt = asks_.find(order.price);
			if (levelIt == asks_.end()) {
				return;
			}

			PriceLevel& level = levelIt->second;
			if (order.prev != kInvalidOrderIndex) {
				orders_.get(order.prev).next = order.next;
			} else {
				level.head = order.next;
			}

			if (order.next != kInvalidOrderIndex) {
				orders_.get(order.next).prev = order.prev;
			} else {
				level.tail = order.prev;
			}

			if (level.head == kInvalidOrderIndex) {
				asks_.erase(levelIt);
			}
		}

		order.prev = kInvalidOrderIndex;
		order.next = kInvalidOrderIndex;
	}

	void remove_order(OrderIndex index) {
		if (!orders_.contains(index)) {
			return;
		}

		Order& order = orders_.get(index);
		detach_from_level(index);
		orderIdToIndex_.erase(order.id);
		order.active = false;
		orders_.release(index);
	}

	MemoryPool<Order, Capacity> orders_{};
	std::map<Price, PriceLevel, std::greater<Price>> bids_{};
	std::map<Price, PriceLevel, std::less<Price>> asks_{};
	std::unordered_map<OrderId, OrderIndex> orderIdToIndex_{};
};
