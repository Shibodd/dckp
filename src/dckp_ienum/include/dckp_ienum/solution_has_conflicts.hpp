#pragma once

#include <Eigen/Dense>

#include <dckp_ienum/instance.hpp>
#include <dckp_ienum/types.hpp>
#include <dckp_ienum/profiler.hpp>

namespace dckp_ienum {

template <typename BoolVector>
bool solution_has_conflicts(const dckp_ienum::Instance& instance, const BoolVector& x) {
    profiler::ScopedTicToc tictoc("solution_has_conflicts");

    for (conflict_index_t i = 0; i < instance.conflicts().size(); ++i) {
        const InstanceConflict& conflict = instance.conflicts().at(i);
        if (x[conflict.i] && x[conflict.j]) {
            std::cerr << "conflict between " << instance.s2o_index(conflict.i) << " and " << instance.s2o_index(conflict.j) << "\n";
            return true;
        }
    }

    return false;
}

} // namespace dckp_ienum