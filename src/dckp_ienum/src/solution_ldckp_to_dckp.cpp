#include "dckp_ienum/ldckp_solver.hpp"
#include <dckp_ienum/solution_ldckp_to_dckp.hpp>
#include <dckp_ienum/profiler.hpp>
#include <dckp_ienum/types.hpp>

namespace dckp_ienum {

void solution_ldckp_to_dckp(const dckp_ienum::Instance& instance, const LdckpResult& result, Solution& solution) {
    profiler::tic("solution_ldckp_to_dckp");

    // Get a feasible solution to the unconstrained binary knapsack problem

    solution.x = result.x_opt.cast<bool>();
    solution.ub = std::min(solution.ub, static_cast<int_profit_t>(result.profit_opt));

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

    solution.p = solution.x.cast<int_profit_t>().matrix().dot(instance.profits().matrix());
    solution.w = solution.x.cast<int_weight_t>().matrix().dot(instance.weights().matrix());
    solution.lb = std::max(solution.lb, solution.p);

    profiler::toc("solution_ldckp_to_dckp");
}

} // namespace dckp_ienum