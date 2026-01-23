#include <dckp_ienum/solution_ldckp_to_dckp.hpp>
#include <dckp_ienum/profiler.hpp>
#include <dckp_ienum/types.hpp>

namespace dckp_ienum {

Eigen::ArrayX<bool> solution_ldckp_to_dckp(const dckp_ienum::Instance& instance, const Eigen::ArrayX<float_t>& x) {
    profiler::tic("solution_ldckp_to_dckp");

    // Get a feasible solution to the unconstrained binary knapsack problem
    Eigen::ArrayX<bool> solution = x.floor().cast<bool>();

    // Satisfy conflict constraints by dropping items
    for (const InstanceConflict& conflict : instance.conflicts()) {
        // If this constraint is unsatisfied
        if (solution(conflict.i) && solution(conflict.j)) {
            // Greedily choose which to keep by comparing costs.

            const auto profit_i = instance.items().at(conflict.i).p;
            const auto profit_j = instance.items().at(conflict.j).p;
            
            const auto todrop = profit_i < profit_j? conflict.i : conflict.j;

            solution(todrop) = false;
        }
    }

    profiler::toc("solution_ldckp_to_dckp");
    return solution;
}

} // namespace dckp_ienum