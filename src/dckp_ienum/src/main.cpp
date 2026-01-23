#include <filesystem>
#include <iostream>
#include <chrono>
#include <fenv.h>

#include <dckp_ienum/instance.hpp>
#include <dckp_ienum/ldckp_solver.hpp>

int main() {
    feenableexcept(FE_INVALID);

    dckp_ienum::Instance instance;
    instance.parse("../../Instances/KPCG_instances/R10/BPPC_2_0_8.txt_0.2");
    
    auto start = std::chrono::steady_clock::now();
    auto result = dckp_ienum::solve_ldckp(instance);
    auto end = std::chrono::steady_clock::now();
    
    std::cout << "UB: " << result.L_opt << std::endl;
    std::cout << "us: " << std::chrono::duration_cast<std::chrono::microseconds>(end - start).count() << std::endl;
    std::cout << "k: " << result.k << std::endl;
}