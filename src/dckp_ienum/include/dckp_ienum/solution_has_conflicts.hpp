#pragma once

#include <Eigen/Dense>

#include <dckp_ienum/instance.hpp>
#include <dckp_ienum/types.hpp>

namespace dckp_ienum {

bool solution_has_conflicts(const dckp_ienum::Instance& instance, const Eigen::ArrayX<bool>& x);

} // namespace dckp_ienum