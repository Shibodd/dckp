#include <dckp_ienum/solution_check.hpp>
#include <dckp_ienum/profiler.hpp>
#include <dckp_ienum/types.hpp>

namespace dckp_ienum {

CheckResult solution_check(const dckp_ienum::Instance& instance, const Eigen::ArrayX<bool>& x) {
    profiler::tic("solution_check");

    if (x.size() != instance.parameters().n) {
        throw "dude.";
    }

    CheckResult ans;
    
    ans.profit = 0;
    ans.weight = 0;
    for (item_index_t i = 0; i < instance.parameters().n; ++i) {
        if (x(i)) {
            ans.profit += instance.items().at(i).p;
            ans.weight += instance.items().at(i).w;
        }
    }
    
    ans.conflicts = 0;
    for (conflict_index_t i = 0; i < instance.parameters().n; ++i) {
        const InstanceConflict& conflict = instance.conflicts().at(i);
        if (x(conflict.i) && x(conflict.j)) {
            ++ans.conflicts;
        }
    }

    ans.feasible = ans.conflicts <= 0 && ans.weight <= instance.parameters().c;

    profiler::toc("solution_check");
    
    return ans;
}

} // namespace dckp_ienum