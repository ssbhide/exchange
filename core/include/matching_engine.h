#pragma once

#include <vector>

#include "order_book.h"

class MatchingEngine {
public:
	using ExecutionLog = std::vector<Execution>;

	template <typename ExecutionSink>
	ExecutionLog process_order(const Order& order, ExecutionSink&& sink) {
		ExecutionLog executions;
		Order workingOrder = order;

		switch (workingOrder.action) {
		case OrderAction::Add: {
			book_.match(workingOrder, [&](const Execution& execution) {
				executions.push_back(execution);
				sink(execution);
			});

			if (workingOrder.quantity > 0) {
				workingOrder.remaining = workingOrder.quantity;
				book_.add_order(workingOrder);
			}
			break;
		}
		case OrderAction::Cancel: {
			book_.cancel_order(workingOrder.id);
			break;
		}
		case OrderAction::Modify: {
			book_.modify_order(workingOrder.id, workingOrder);
			break;
		}
		}

		return executions;
	}

	ExecutionLog process_order(const Order& order) {
		return process_order(order, [](const Execution&) {});
	}

	OrderBook<>& book() {
		return book_;
	}

	const OrderBook<>& book() const {
		return book_;
	}

private:
	OrderBook<> book_{};
};
