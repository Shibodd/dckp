#pragma once

#include <dckp_ienum/instance.hpp>
#include <iostream>

namespace dckp_ienum {

std::ostream& solution_print(std::ostream& os, const Solution& solution, const dckp_ienum::Instance& instance);

} // namespace dckp_ienum