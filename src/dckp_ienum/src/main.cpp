#include <filesystem>
#include <iostream>
#include <chrono>
#include <fenv.h>

#include <dckp_ienum/solution_ldckp_to_dckp.hpp>
#include <dckp_ienum/solution_check.hpp>
#include <dckp_ienum/ldckp_solver.hpp>
#include <dckp_ienum/instance.hpp>
#include <dckp_ienum/profiler.hpp>


void check_constraints(const dckp_ienum::Instance& before, const dckp_ienum::Instance& after) {
    for (unsigned int i = 0; i < before.conflicts().size(); ++i) {
        auto cbefore = before.conflicts().at(i);
        auto cafter = after.conflicts().at(i);

        auto ibefore = before.items().at(cbefore.i);
        auto iafter = after.items().at(cafter.i);

        auto jbefore = before.items().at(cbefore.j);
        auto jafter = after.items().at(cafter.j);

        bool ok = ibefore.id == iafter.id;
        ok &= jbefore.id == jafter.id;

        if (not ok) {
            std::cout << "cbefore: " << cbefore << std::endl;
            std::cout << "cafter: " << cafter << std::endl;

            std::cout << "ibefore: " << ibefore << std::endl;
            std::cout << "iafter: " << iafter << std::endl;

            std::cout << "jbefore: " << jbefore << std::endl;
            std::cout << "jafter: " << jafter << std::endl;

            throw "check_constraints failed";
        }
    } 
}

int main() {
    feenableexcept(FE_INVALID);

    dckp_ienum::Instance instance;
    instance.parse("../../Instances/KPCG_instances/R10/BPPC_2_0_8.txt_0.2");
    
    if (true) {
        // sanity check on instance
        dckp_ienum::Instance before = instance;
        instance.sort_items();
        check_constraints(before, instance);
    } else {
        instance.sort_items();
    }


    auto result = dckp_ienum::solve_ldckp(instance);
    auto lb_soln_start = dckp_ienum::solution_ldckp_to_dckp(instance, result.x_opt);
    if (not dckp_ienum::solution_check(instance, lb_soln_start).feasible) {
        throw "bug";
    }

    dckp_ienum::profiler::print_stats(std::cout);

    std::cout << "UB: " << result.L_opt << std::endl;
    std::cout << "k: " << result.k << std::endl;
}