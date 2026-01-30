#include "dckp_ienum/conflicts.hpp"
#include "dckp_ienum/profiler.hpp"
#include <algorithm>
#include <initializer_list>

#include <dckp_ienum/solution_has_conflicts.hpp>
#include <dckp_ienum/fkp_solver.hpp>
#include <dckp_ienum/dckp_ienum_solver.hpp>
#include <dckp_ienum/ldckp_solver.hpp>
#include <dckp_ienum/types.hpp>
#include <limits>

namespace dckp_ienum {


using ItemSet = std::vector<item_index_t>;

struct IEnumNode {
    std::vector<bool> x;
    ItemSet conflict_set;
    int_profit_t profit = 0;
    int_weight_t weight = 0;
    int_profit_t ub = std::numeric_limits<int_profit_t>::max();

    IEnumNode() {}

    IEnumNode(const IEnumNode& parent, item_index_t j, int_profit_t profit, int_weight_t weight, const ItemSet& jth_conflict_set, int_profit_t ub)
        : profit(profit),
          weight(weight),
          ub(ub)
    {
        // Extend solution
        x.reserve(j + 1);
        x = parent.x;
        x.resize(j + 1, false);
        x[j] = true;

        conflict_set.reserve(parent.conflict_set.size() + jth_conflict_set.size());
        std::set_union(
            parent.conflict_set.begin(), parent.conflict_set.end(),
            jth_conflict_set.begin(), jth_conflict_set.end(),
            std::back_inserter(conflict_set)
        );
    }

    friend std::ostream& operator<<(std::ostream& os, const IEnumNode& node) {
        os << "p=" << node.profit << ", w=" << node.weight;
        return os;
    }
};

void solve_dckp_ienum(const dckp_ienum::Instance& instance, int_profit_t lb, Solution& soln, std::atomic<bool>* stop_token, const std::function<void(const Solution&)>& solution_callback) {
    profiler::ScopedTicToc ticto("solve_dckp_ienum");

    std::vector<IEnumNode> next_fifo;
    std::vector<IEnumNode> current_fifo;

    std::vector<item_index_t> jth_conflict_set;

    soln.p = 0;
    soln.w = 0;
    soln.ub = 0;
    std::fill(soln.x.begin(), soln.x.end(), false);

    // Push a node with no choices made
    current_fifo.emplace_back();
    
    auto jp1th_rconflicts_begin = instance.rconflicts().begin();
    auto rconflicts_end = instance.rconflicts().end();

    auto jth_conflicts_begin = instance.conflicts().begin();
    auto conflicts_end = instance.conflicts().end();

    bool solution_found = false;

    for (item_index_t j = 0; j < instance.num_items(); ++j) {
        if (*stop_token) {
            break;
        }

        std::cout << "level " << j << ", " << current_fifo.size() << " nodes" << std::endl;

        /* Termination of unfeasible problems */
        if (current_fifo.empty()) {
            break;
        }

        advance_conflict_iterator(j, jth_conflicts_begin, conflicts_end);
        advance_conflict_iterator(j, jp1th_rconflicts_begin, rconflicts_end);

        /* Update jth_conflict_set with the items not yet in the knapsack that conflict with item j */
        {
            profiler::ScopedTicToc tictoc("jth_conflict_set");
            jth_conflict_set.clear();

            for (auto conflict_it = jth_conflicts_begin; conflict_it != conflicts_end && conflict_it->i == j; ++conflict_it) {
                item_index_t conflicting_item = conflict_it->j;

                auto it = std::lower_bound(jth_conflict_set.begin(), jth_conflict_set.end(), conflicting_item);
                if (it == jth_conflict_set.end() || *it != conflicting_item) {
                    jth_conflict_set.insert(it, conflicting_item);
                }
            }
        }

        auto max_ub_it = std::max_element(current_fifo.begin(), current_fifo.end(), [](const IEnumNode& a, const IEnumNode& b) {
            return a.ub < b.ub;
        });
        int_profit_t max_ub = std::numeric_limits<int_profit_t>::max();
        if (max_ub_it != current_fifo.end()) {
            max_ub = max_ub_it->ub;
        }

        for (std::size_t parent_idx = 0; parent_idx < current_fifo.size(); ++parent_idx) {
            if (*stop_token) {
                break;
            }

            auto& parent = current_fifo[parent_idx];

            /*
            C3 can only be checked when we have the full FIFO,
            but C1,C2,C4 can be checked before.

            Here we terminate the parent node if C3 is not satisfied.
            Then, we "terminate" the children nodes that do not satisfy C1,C2,C4 by not even creating them.
            */

            // Check C3 on the parent node
            if (true) {
                profiler::ScopedTicToc tictoc("C3");

                bool dominated = false;
                for (std::size_t other_node_idx = 0; other_node_idx < current_fifo.size(); ++other_node_idx) {
                    if (other_node_idx == parent_idx) {
                        continue;
                    }
    
                    auto& other_node = current_fifo[other_node_idx];
                    if (other_node.profit >= parent.profit && other_node.weight <= parent.weight) {
                        // Is the conflict set of the other node a subset of the current node?
                        dominated = std::includes(
                            parent.conflict_set.begin(), parent.conflict_set.end(),
                            other_node.conflict_set.begin(), other_node.conflict_set.end()
                        );
                    }

                    if (dominated) {
                        break;
                    }
                }

                if (dominated) {
                    // std::cout << "dominated" << std::endl;
                    continue;
                }
            }

            if (parent.profit > soln.p) {
                solution_found = true;
                soln.w = parent.weight;
                soln.p = parent.profit;
                soln.x = parent.x;
                soln.ub = max_ub;

                soln.x.resize(instance.num_items(), false);
                solution_callback(soln);
            }

            // Don't add children if this is the last item
            if (j == instance.num_items() - 1) {
                continue;
            }

            bool add_true = false;

            // Check C1,C2 to decide whether we should add the j-th item
            int_profit_t p = parent.profit + instance.profits()(j);
            int_weight_t w = parent.weight + instance.weights()(j);
            do {
                // Fast-path
                if (p > max_ub) {
                    break;
                }
    
                // C1
                if (w > instance.capacity()) {
                    break;
                }
    
                // C2
                {
                    profiler::ScopedTicToc tictoc("C2");
                    // It is enough to check whether item j is in the parent's conflict set
                    if (std::binary_search(parent.conflict_set.begin(), parent.conflict_set.end(), j)) {
                        break;
                    }
                }
                add_true = true;
            } while (false);

            // C4
            int_profit_t ub_true = add_true? std::min(solve_fkp_fast(instance, j+1, p, w).ub, parent.ub) : 0;
            int_profit_t ub_false = std::min(solve_fkp_fast(instance, j+1, parent.profit, parent.weight).ub, parent.ub);

            if (add_true && ub_true >= lb) {
                profiler::ScopedTicToc tictoc("create_true_node");
                next_fifo.emplace_back(parent, j, p, w, jth_conflict_set, ub_true);

                if (p > lb) {
                    lb = p;
                }
            }
            if (ub_false >= lb) {
                profiler::ScopedTicToc tictoc("create_false_node");
                next_fifo.push_back(parent);
                next_fifo.back().ub = ub_false;
            }
        }

        // Swap the two FIFOs
        current_fifo.swap(next_fifo);
        next_fifo.clear();
        next_fifo.reserve(current_fifo.size() * 2);
    }

    if (not solution_found) {
        soln.x.clear();
        soln.x.resize(instance.num_items(), false);
        soln.p = 0;
        soln.w = 0;
        soln.ub = 0;
        solution_callback(soln);
    }
}

} // namespace dckp_ienum