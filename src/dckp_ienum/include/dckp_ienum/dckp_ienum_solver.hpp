#pragma once

#include "dckp_ienum/types.hpp"
#include <dckp_ienum/instance.hpp>

namespace dckp_ienum {

void solve_dckp_ienum(const dckp_ienum::Instance& instance, int_profit_t lb, Solution& soln);

} // namespace dckp_ienum