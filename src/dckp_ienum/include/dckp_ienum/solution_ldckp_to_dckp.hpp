#pragma once

#include <Eigen/Dense>
#include <dckp_ienum/instance.hpp>

namespace dckp_ienum {

Eigen::ArrayX<bool> solution_ldckp_to_dckp(const dckp_ienum::Instance& instance, const Eigen::ArrayX<float_t>& x);

} // namespace dckp_ienum