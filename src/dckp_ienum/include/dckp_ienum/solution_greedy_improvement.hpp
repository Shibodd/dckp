#include <dckp_ienum/conflicts.hpp>
#include <dckp_ienum/instance.hpp>
#include <dckp_ienum/types.hpp>

namespace dckp_ienum {

void solution_greedy_improve(const Instance& instance, Solution& soln, item_index_t jp1, ConflictConstIterator& jp1th_rconflicts_iterator, ConflictConstIterator jp1th_conflicts_begin);
void solution_greedy_remove_conflicts(const Instance& instance, Solution& soln, item_index_t jp1, ConflictConstIterator jth_rconflicts_begin);

} // namespace dckp_ienum