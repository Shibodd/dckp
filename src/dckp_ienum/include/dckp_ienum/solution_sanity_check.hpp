#pragma once

#include <dckp_ienum/instance.hpp>
#include <iostream>

namespace dckp_ienum {

void solution_sanity_check(const Solution& solution, const dckp_ienum::Instance& instance, bool check_conflicts = true);

} // namespace dckp_ienum