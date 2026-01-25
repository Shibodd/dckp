#include <filesystem>
#include <iostream>
#include <chrono>
#include <fenv.h>

#include <dckp_ienum/solution_has_conflicts.hpp>
#include <dckp_ienum/solution_ldckp_to_dckp.hpp>
#include <dckp_ienum/dckp_greedy_solver.hpp>
#include <dckp_ienum/solution_print.hpp>
#include <dckp_ienum/ldckp_solver.hpp>
#include <dckp_ienum/instance.hpp>
#include <dckp_ienum/profiler.hpp>
#include <dckp_ienum/types.hpp>

int main() {
    feenableexcept(FE_INVALID);

    dckp_ienum::Instance instance;
    instance.parse("../../Instances/KPCG_instances/R10/BPPC_2_0_8.txt_0.2");
    instance.sort_items();

    auto result = dckp_ienum::solve_ldckp(instance);

    dckp_ienum::Solution solution;
    dckp_ienum::solution_ldckp_to_dckp(instance, result, solution);
    dckp_ienum::solve_dckp_greedy(instance, solution);

    dckp_ienum::profiler::print_stats(std::cout);
}