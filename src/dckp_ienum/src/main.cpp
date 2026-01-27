#include "dckp_ienum/dckp_ienum_solver.hpp"
#include <filesystem>
#include <iostream>
#include <chrono>
#include <fenv.h>

#include <dckp_ienum/solution_has_conflicts.hpp>
#include <dckp_ienum/solution_ldckp_to_dckp.hpp>
#include <dckp_ienum/solution_sanity_check.hpp>
#include <dckp_ienum/dckp_hillclimb_solver.hpp>
#include <dckp_ienum/dckp_greedy_solver.hpp>
#include <dckp_ienum/solution_print.hpp>
#include <dckp_ienum/ldckp_solver.hpp>
#include <dckp_ienum/instance.hpp>
#include <dckp_ienum/profiler.hpp>
#include <dckp_ienum/types.hpp>

#include <ostream>
#include <signal.h>

void my_handler(int s)
{
    dckp_ienum::profiler::print_stats(std::cout);
    exit(1);
}

int main() {
    signal(SIGINT, my_handler);

    feenableexcept(FE_INVALID);

    

    dckp_ienum::Instance instance;
    instance.parse("../../Instances/KPCG_instances/C1/BPPC_4_0_2.txt_0.9");
    instance.sort_items();

    std::cout << "n: " << instance.num_items() << std::endl;
    std::cout << "m: " << instance.conflicts().size() << std::endl;
    std::cout << "c: " << instance.capacity() << std::endl;

    auto result = dckp_ienum::solve_ldckp(instance, {}, 10000, 1.0);


    dckp_ienum::Solution solution;
    dckp_ienum::solution_ldckp_to_dckp(instance, result, solution);
    std::cout << "ldckp ";
    dckp_ienum::solution_print(std::cout, solution, instance) << '\n' << std::endl;

    dckp_ienum::solve_dckp_greedy(instance, solution);
    std::cout << "greedy ";
    dckp_ienum::solution_print(std::cout, solution, instance) << '\n' << std::endl;

    std::cout << "hillclimb " << dckp_ienum::solve_dckp_hillclimb(instance, solution) << std::endl;
    std::cout << "hillclimb ";
    dckp_ienum::solution_print(std::cout, solution, instance) << '\n' << std::endl;

    dckp_ienum::solution_sanity_check(solution, instance);

    dckp_ienum::solve_dckp_ienum(instance, solution.p, solution);
    dckp_ienum::solution_sanity_check(solution, instance);

    dckp_ienum::profiler::print_stats(std::cout);

    dckp_ienum::solution_print(std::cout, solution, instance) << std::endl;
}