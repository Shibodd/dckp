#include <filesystem>
#include <iostream>
#include <chrono>
#include <fenv.h>

#include <dckp_ienum/solution_ldckp_to_dckp.hpp>
#include <dckp_ienum/solution_check.hpp>
#include <dckp_ienum/ldckp_solver.hpp>
#include <dckp_ienum/instance.hpp>
#include <dckp_ienum/profiler.hpp>

int main() {
    feenableexcept(FE_INVALID);

    dckp_ienum::Instance instance;
    instance.parse("../../Instances/KPCG_instances/R10/BPPC_2_0_8.txt_0.2");

    auto result = dckp_ienum::solve_ldckp(instance);
    auto lb_soln_start = dckp_ienum::solution_ldckp_to_dckp(instance, result.x_opt);
    if (not dckp_ienum::solution_check(instance, lb_soln_start).feasible) {
        throw "bug";
    }

    dckp_ienum::profiler::print_stats(std::cout);

    std::cout << "UB: " << result.L_opt << std::endl;
    std::cout << "k: " << result.k << std::endl;
}