#include "dckp_ienum/conflicts.hpp"
#include <dckp_ienum/solution_greedy_improvement.hpp>

namespace dckp_ienum {

void solution_greedy_improve(const Instance& instance, Solution& soln, item_index_t jp1, ConflictConstIterator& jp1th_rconflicts_iterator, ConflictConstIterator jp1th_conflicts_begin) {
    auto ith_conflicts_it = jp1th_conflicts_begin;
    auto conflicts_end = instance.conflicts().end();
    auto rconflicts_end = instance.rconflicts().end();

    for (item_index_t i = jp1; i < instance.num_items(); ++i) {
        // Advance conflict iterators to ith-item conflicts
        advance_conflict_iterator(i, jp1th_rconflicts_iterator, rconflicts_end);
        advance_conflict_iterator(i, ith_conflicts_it, conflicts_end);

        if (soln.x[i]) {
            continue;
        }

        // Check conflicts with both items < i and items > i (they may be set by the LDCKP problem)
        if (check_conflict(soln.x, i, jp1th_rconflicts_iterator, rconflicts_end) || check_conflict(soln.x, i, ith_conflicts_it, conflicts_end)) {
            continue;
        }

        int_weight_t w_new = soln.w + instance.weight(i);
        if (w_new > instance.capacity()) {
            continue;
        }

        soln.w = w_new;
        soln.p = soln.p + instance.profit(i);
        soln.x[i] = true;
    }
}

void solution_greedy_remove_conflicts(const Instance& instance, Solution& soln, item_index_t jp1, ConflictConstIterator jth_rconflicts_begin) {
    // Greedily drop items (idx > j) that break conflicts (drop the ones with worse p/w ratio)
    auto rconflicts_rit = instance.rconflicts().rbegin();
    const auto rconflicts_rend = std::reverse_iterator(jth_rconflicts_begin);

    for (item_index_t _i = instance.num_items(); _i > jp1; --_i) {
        item_index_t i = _i - 1;

        if (not soln.x[i]) {
            continue;
        }

        advance_reverse_conflict_iterator(i, rconflicts_rit, rconflicts_rend);
        if (rconflicts_rit == rconflicts_rend) {
            break;
        }

        if (check_conflict(soln.x, i, rconflicts_rit, rconflicts_rend)) {
            soln.x[i] = false;
            soln.p -= instance.profit(i);
            soln.w -= instance.weight(i);
        }
    }
}

} // namespace dckp_ienum
