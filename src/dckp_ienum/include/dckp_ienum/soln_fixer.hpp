#pragma once

#include <Eigen/Dense>
#include <dckp_ienum/instance.hpp>

namespace dckp_ienum {

Eigen::ArrayX<bool> soln_fixer(const dckp_ienum::Instance& instance, const Eigen::ArrayX<float_t>& x);

} // namespace dckp_ienum