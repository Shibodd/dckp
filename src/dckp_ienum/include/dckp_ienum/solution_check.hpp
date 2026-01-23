#pragma once

#include <Eigen/Dense>

#include <dckp_ienum/instance.hpp>
#include <dckp_ienum/types.hpp>

namespace dckp_ienum {

struct CheckResult {
    conflict_index_t conflicts;
    int_profit_t profit;
    int_weight_t weight;
    bool feasible;

    friend std::ostream& operator<<(std::ostream& os, const CheckResult& result) {
        os << (result.feasible? "Feasible" : "Infeasible") 
           << " solution with profit " << result.profit
           << " and weight " << result.weight;
        
        if (not result.feasible) {
            os << " and " << result.conflicts << " conflicts";
        }
        return os << ".";
    }
};

CheckResult solution_check(const dckp_ienum::Instance& instance, const Eigen::ArrayX<bool>& x);

} // namespace dckp_ienum