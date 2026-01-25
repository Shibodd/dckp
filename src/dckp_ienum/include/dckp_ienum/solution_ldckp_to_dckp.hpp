#pragma once

#include <Eigen/Dense>
#include <dckp_ienum/ldckp_solver.hpp>
#include <dckp_ienum/instance.hpp>

namespace dckp_ienum {

void solution_ldckp_to_dckp(const dckp_ienum::Instance& instance, const LdckpResult& ldckp_result, Solution& solution);

} // namespace dckp_ienum