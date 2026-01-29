#include <dckp_ienum/solution_has_conflicts.hpp>
#include <dckp_ienum/dckp_greedy_solver.hpp>
#include <dckp_ienum/profiler.hpp>

namespace dckp_ienum {

void solve_dckp_greedy(const dckp_ienum::Instance& instance, Solution& soln) {
    profiler::tic("solve_dckp_greedy");

    // Iterating over items by p/w ratio, add each that fits
    for (item_index_t i = 0; i < instance.num_items(); ++i) {
        // Skip items that are already in the knapsack
        if (soln.x[i])
            continue;

        // If we add this item, will the solution still be feasible?
        int_profit_t p_new = soln.p + instance.profit(i);
        int_weight_t w_new = soln.w + instance.weight(i);
        if (w_new > instance.capacity()) {
            continue;
        }

        soln.x[i] = true;
        if (solution_has_conflicts(instance, soln.x)) {
            soln.x[i] = false;
            continue;
        }

        // Solution is feasible and obviously better - update the solution
        soln.p = p_new;
        soln.w = w_new;
    }

    profiler::toc("solve_dckp_greedy");
}

} // namespace dckp_ienum