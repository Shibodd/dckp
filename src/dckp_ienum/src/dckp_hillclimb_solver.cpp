#include "dckp_ienum/dckp_hillclimb_solver.hpp"
#include "dckp_ienum/profiler.hpp"
#include "dckp_ienum/solution_has_conflicts.hpp"
#include "dckp_ienum/types.hpp"
#include <dckp_ienum/instance.hpp>
#include <dckp_ienum/solution_print.hpp>
namespace dckp_ienum {

static inline bool swap_move(const dckp_ienum::Instance& instance, Solution& soln, item_index_t i, item_index_t j) {
    // The other item should be better (i.e. improve the solution)
    if (instance.profit(j) <= instance.profit(i)) {
        return false;
    }

    // Profit after the move should be LTE to the upper bound
    int_profit_t new_profit = (soln.p - instance.profit(i)) + instance.profit(j);
    if (new_profit > soln.ub) {
        return false;
    }

    // The other item should fit
    int_weight_t new_weight = (soln.w - instance.weight(i)) + instance.weight(j);
    if (new_weight > instance.capacity()) {
        return false;
    }

    // The other item should not break any conflict
    soln.x(i) = false;
    soln.x(j) = true;
    if (solution_has_conflicts(instance, soln.x)) {
        // Undo modification to x
        soln.x(i) = true;
        soln.x(j) = false;
        return false;
    }

    soln.p = new_profit;
    soln.w = new_weight;
    return true;
}

static inline bool add_move(const dckp_ienum::Instance& instance, Solution& soln, item_index_t i) {
    // Profit after the move should be LTE to the upper bound
    int_profit_t new_profit = soln.p + instance.profit(i);
    if (new_profit > soln.ub) {
        return false;
    }

    // The new item should fit
    int_weight_t new_weight = soln.w + instance.weight(i);
    if (new_weight > instance.capacity()) {
        return false;
    }

    // The new item should not break any conflict
    soln.x(i) = true;
    if (solution_has_conflicts(instance, soln.x)) {
        // Undo modification to x
        soln.x(i) = false;
        return false;
    }

    soln.p = new_profit;
    soln.w = new_weight;
    return true;

}

static inline bool dckp_hillclimb_step(const dckp_ienum::Instance& instance, Solution& soln, HillclimbStats& stats) {
    profiler::ScopedTicToc tictoc("dckp_hillclimb_step");

    // Local search with hillclimb strategy
    // Possible moves: add one item to the knapsack, swap two items

    for (item_index_t i = 0; i < instance.num_items(); ++i) {
        if (soln.x(i)) {
            // Iterate over items not in knapsack
            for (unsigned int j = 0; j < instance.num_items(); ++j) {
                if (i != j || soln.x(j))
                    continue;
                
                // MOVE: swap i and j
                if (swap_move(instance, soln, i, j)) {
                    ++stats.swaps;
                    return true;
                }
            }
        } else {
            // MOVE: add ith item to knapsack
            if (add_move(instance, soln, i)) {
                ++stats.adds;
                return true;
            }
        }
    }

    return false; // no improvement
}

HillclimbStats solve_dckp_hillclimb(const dckp_ienum::Instance& instance, Solution& soln) {
    HillclimbStats stats;
    while (dckp_hillclimb_step(instance, soln, stats));
    return stats;
}

} // namespace dckp_ienum