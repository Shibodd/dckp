#include <dckp_ienum/solution_has_conflicts.hpp>
#include <dckp_ienum/profiler.hpp>

namespace dckp_ienum {

bool solution_has_conflicts(const dckp_ienum::Instance& instance, const Eigen::ArrayX<bool>& x) {
    profiler::tic("solution_has_conflicts");

    for (conflict_index_t i = 0; i < instance.num_items(); ++i) {
        const InstanceConflict& conflict = instance.conflicts().at(i);
        if (x(conflict.i) && x(conflict.j)) {
            return true;
        }
    }

    profiler::toc("solution_has_conflicts");
    return false;
}

} // namespace dckp_ienum