#pragma once

#include <Eigen/Dense>
#include <dckp_ienum/types.hpp>
#include <dckp_ienum/instance.hpp>

namespace dckp_ienum {

struct LdckpResult {
    Eigen::VectorX<float_t> x_opt;
    Eigen::VectorX<float_t> lambda_opt;

    float_t profit_opt;
    float_t L_opt;
    int_weight_t weight_opt;
    unsigned int k;

    LdckpResult(std::size_t n, std::size_t m);
};

LdckpResult solve_ldckp(const Instance& instance);

} // namespace dckp_ienum