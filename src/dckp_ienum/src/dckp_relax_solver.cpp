#include "dckp_ienum/profiler.hpp"
#include <variant>

#include <dckp_ienum/conflicts.hpp>
#include <dckp_ienum/dckp_relax_solver.hpp>
#include <dckp_ienum/ldckp_solver.hpp>
#include <dckp_ienum/fkp_solver.hpp>
#include <dckp_ienum/solution_greedy_improvement.hpp>

namespace dckp_ienum {

void solve_dckp_relax(const Instance& instance, Solution& solution, bool use_ldckp, std::atomic<bool>* stop_token, const std::function<void(const Solution&)>& solution_callback) {
    profiler::ScopedTicToc tictoc("solve_dckp_relax");

    if (use_ldckp) {
        auto result = dckp_ienum::solve_ldckp(instance, solution.x, 0, 0, 0, instance.rconflicts().begin(), dckp_ienum::LdckpSolverParams {});
        result.convert(instance, solution, 0);
    } else {
        auto result = solve_fkp_fast(instance, 0, 0, 0);
        result.convert(instance, solution, 0);
    }

    if (*stop_token) {
        solution.p = 0;
        solution.w = 0;
        std::fill(solution.x.begin(), solution.x.end(), false);
        solution_callback(solution);
        return;
    }

    solution_greedy_remove_conflicts(instance, solution, 0, instance.rconflicts().begin());

    if (*stop_token) {
        solution_callback(solution);
        return;
    }

    auto rconflicts_it = instance.rconflicts().begin();
    solution_greedy_improve(instance, solution, 0, rconflicts_it, instance.conflicts().begin());

    solution_callback(solution);
}

} // namespace dckp_ienum