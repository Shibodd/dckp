#pragma once

#include <Eigen/Dense>

#include <dckp_ienum/conflicts.hpp>
#include <dckp_ienum/instance.hpp>
#include <dckp_ienum/types.hpp>
#include <limits>

namespace dckp_ienum {

struct LdckpSolverParams {
    float_t alpha = 2.75;
    std::size_t k_max = 5;
    float_t eps = 1e-3;
};

struct LdckpResult {
    Eigen::VectorX<float_t> x;
    float_t ub = std::numeric_limits<float_t>::max();
    void convert(const Instance& instance, Solution &soln, item_index_t j);
};

LdckpResult solve_ldckp(const Instance& instance, std::vector<bool> fixed_items, item_index_t jp1, int_profit_t fixed_items_p, int_weight_t fixed_items_w, ConflictConstIterator jth_rconflict_iterator, const LdckpSolverParams& params);

} // namespace dckp_ienum