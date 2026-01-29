#pragma once

#include <dckp_ienum/profiler.hpp>
#include <dckp_ienum/instance.hpp>
#include <dckp_ienum/types.hpp>

namespace dckp_ienum {

using ConflictList = std::vector<InstanceConflict>;
using ConflictConstIterator = ConflictList::const_iterator;
using ConflictConstReverseIterator = ConflictList::const_reverse_iterator;

static inline ConflictConstIterator find_conflict_iterator(const ConflictList& conflicts, item_index_t item_index) {
    return std::lower_bound(conflicts.begin(), conflicts.end(), item_index, [](const InstanceConflict& conflict, item_index_t j) {
        return conflict.i < j;
    });
}

// Advance conflict iterator to the beginning of the conflicts of the item with the specified index.
static inline void advance_conflict_iterator(item_index_t item_idx, ConflictConstIterator& it, ConflictConstIterator end) {
    profiler::ScopedTicToc tictoc("advance_rconflict_iterator");

    while (it != end && it->i < item_idx) {
        ++it;   
    }
}
static inline void advance_reverse_conflict_iterator(item_index_t item_idx, ConflictConstReverseIterator& it, ConflictConstReverseIterator end) {
    profiler::ScopedTicToc tictoc("advance_reverse_rconflict_iterator");

    while (it != end && it->i > item_idx) {
        ++it;
    }
}

/*
Starting from the rconflict iterator, check conflits between specified item and the items already in the knapsack.
The iterator is advanced until a conflict is found or when the conflicts of the specified item are exhausted.
*/ 
template <typename Iterator>
static inline bool check_conflict(
        const std::vector<bool>& items,
        item_index_t item_idx,
        Iterator& it,
        Iterator rconflicts_end)
{
    static_assert(std::is_same_v<Iterator, ConflictConstReverseIterator> || std::is_same_v<Iterator, ConflictConstIterator>);

    profiler::ScopedTicToc tictoc("check_conflict");

    for (; it != rconflicts_end; ++it) {
        if (it->i != item_idx) {
            break;
        }

        if (items[it->j]) {
            return true;
        }
    }
    return false;
}

} // namespace dckp_ienum