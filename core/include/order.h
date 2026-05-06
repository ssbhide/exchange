#pragma once

#include "types.h"

struct Order {
    OrderId id{};
    Side side{Side::Bid};
    OrderAction action{OrderAction::Add};
    Price price{};
    Quantity quantity{};
    Quantity remaining{};
    OrderIndex prev{kInvalidOrderIndex};
    OrderIndex next{kInvalidOrderIndex};
    bool active{false};
};