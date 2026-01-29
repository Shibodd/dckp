#include "dckp_ienum/dckp_hillclimb_solver.hpp"
#include "dckp_ienum/conflicts.hpp"
#include "dckp_ienum/profiler.hpp"
#include "dckp_ienum/solution_has_conflicts.hpp"
#include "dckp_ienum/solution_sanity_check.hpp"
#include "dckp_ienum/types.hpp"
#include <dckp_ienum/instance.hpp>
#include <dckp_ienum/solution_print.hpp>
#include <variant>
namespace dckp_ienum {

class AddMove;
class SwapMove;

#define MOVE_LIST AddMove, SwapMove
using Move = std::variant<MOVE_LIST>;

class AddMove {
    item_index_t i;
    
public:
    AddMove(item_index_t i) : i(i) {}

    void apply(const Instance& instance, Solution& soln, HillclimbStats& stats) {
        soln.p += instance.profit(i);
        soln.w += instance.weight(i);
        soln.x[i] = true;

        ++stats.adds;
    }
};

class SwapMove {
    item_index_t i;
    item_index_t j;

public:
    SwapMove(item_index_t i, item_index_t j) : i(i), j(j) {}

    void apply(const Instance& instance, Solution& soln, HillclimbStats& stats) {
        soln.p -= instance.profit(i);
        soln.w -= instance.weight(i);

        soln.p += instance.profit(j);
        soln.w += instance.weight(j);

        soln.x[i] = false;
        soln.x[j] = true;

        ++stats.swaps;
    }
};

template <typename T>
inline void generate_moves_impl(const Instance& instance, Solution& soln, const std::function<void (Move, int_profit_t)> &callback);

template <>
inline void generate_moves_impl<AddMove>(const Instance& instance, Solution& soln, const std::function<void (Move, int_profit_t)> &callback) {
    for (item_index_t i = 0; i < instance.num_items(); ++i) {
        if (soln.x[i]) {
            continue;
        }

        int_profit_t p = soln.p + instance.profit(i);
        if (p > soln.ub) {
            continue;
        }

        int_weight_t w = soln.w + instance.weight(i);
        if (w > instance.capacity()) {
            continue;
        }

        ConflictConstIterator conflicts_begin = find_conflict_iterator(instance.conflicts(), i);
        ConflictConstIterator conflicts_end = instance.conflicts().end();
        if (check_conflict(soln.x, i, conflicts_begin, conflicts_end)) {
            continue;
        }

        ConflictConstIterator rconflicts_begin = find_conflict_iterator(instance.rconflicts(), i);
        ConflictConstIterator rconflicts_end = instance.rconflicts().end();
        if (check_conflict(soln.x, i, rconflicts_begin, rconflicts_end)) {
            continue;
        }
        
        callback(AddMove { i }, p);
    }
}

template <>
void generate_moves_impl<SwapMove>(const Instance &instance, Solution &soln, const std::function<void (Move, int_profit_t)> &callback) {
    for (item_index_t i = 0; i < instance.num_items(); ++i) {
        if (not soln.x[i]) {
            continue;
        }

        for (item_index_t j = 0; j < instance.num_items(); ++j) {
            if (i == j) {
                continue;
            }
            if (soln.x[j]) {
                continue;
            }

            int_profit_t p = soln.p - instance.profit(i) + instance.profit(j);
            if (p > soln.ub) {
                continue;
            }

            int_weight_t w = soln.w - instance.weight(i) + instance.weight(j);
            if (w > instance.capacity()) {
                continue;
            }

            soln.x[i] = false;

            ConflictConstIterator conflicts_begin = find_conflict_iterator(instance.conflicts(), j);
            ConflictConstIterator conflicts_end = instance.conflicts().end();
            if (check_conflict(soln.x, j, conflicts_begin, conflicts_end)) {
                soln.x[i] = true;
                continue;
            }

            ConflictConstIterator rconflicts_begin = find_conflict_iterator(instance.rconflicts(), j);
            ConflictConstIterator rconflicts_end = instance.rconflicts().end();
            if (check_conflict(soln.x, j, rconflicts_begin, rconflicts_end)) {
                soln.x[i] = true;
                continue;
            }

            soln.x[i] = true;
            callback(SwapMove { i, j }, p);
        }
    }
}

template <typename... Moves>
static inline void generate_moves_impl_(const Instance &instance, Solution &soln, const std::function<void (Move, int_profit_t)> &callback) {
    (generate_moves_impl<Moves>(instance, soln, callback), ...);
}

static inline void generate_moves(const Instance &instance, Solution &soln, const std::function<void (Move, int_profit_t)> &callback) {
    generate_moves_impl_<MOVE_LIST>(instance, soln, callback);
}


HillclimbStats solve_dckp_hillclimb(const dckp_ienum::Instance& instance, Solution& soln) {
    HillclimbStats stats;

    int_profit_t best_profit;
    std::optional<Move> best_move;

    do {
        best_move.reset();
        best_profit = soln.p;

        generate_moves(instance, soln, [&](Move move, int_profit_t profit) {
            if (profit > best_profit) {
                best_move = move;
                best_profit = profit;
            }
        });

        if (best_move.has_value()) {
            std::visit([&](auto& arg) {
                arg.apply(instance, soln, stats);
            }, *best_move);
        }
    } while(best_move.has_value());
    return stats;
}

} // namespace dckp_ienum