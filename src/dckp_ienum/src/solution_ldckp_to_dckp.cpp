#include "dckp_ienum/ldckp_solver.hpp"
#include <dckp_ienum/solution_ldckp_to_dckp.hpp>
#include <dckp_ienum/profiler.hpp>
#include <dckp_ienum/types.hpp>

namespace dckp_ienum {

void solution_ldckp_to_dckp(const dckp_ienum::Instance& instance, const Eigen::ArrayX<float_t>& x, Solution& solution) {
    profiler::tic("solution_ldckp_to_dckp");

    // Get a feasible solution to the unconstrained binary knapsack problem

    solution.x = x.cwiseEqual(float_t(1.0));
    // solution.ub = std::min(solution.ub, static_cast<int_profit_t>(result.L_opt));

    // Satisfy conflict constraints by dropping items
    for (const InstanceConflict& conflict : instance.conflicts()) {
        // If this constraint is unsatisfied
        if (solution.x(conflict.i) && solution.x(conflict.j)) {
            // Greedily choose which to keep by comparing costs.

            const auto profit_i = instance.profit(conflict.i);
            const auto profit_j = instance.profit(conflict.j);
            
            const auto todrop = profit_i < profit_j? conflict.i : conflict.j;

            solution.x(todrop) = false;
        }
    }

    solution.p = solution.x.select(instance.profits(), 0).sum();
    solution.w = solution.x.select(instance.weights(), 0).sum();

    profiler::toc("solution_ldckp_to_dckp");
}

} // namespace dckp_ienum