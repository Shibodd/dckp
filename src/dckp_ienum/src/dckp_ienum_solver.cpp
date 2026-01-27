#include "dckp_ienum/profiler.hpp"
#include <initializer_list>

#include <dckp_ienum/solution_has_conflicts.hpp>
#include <dckp_ienum/fkp_solver.hpp>
#include <dckp_ienum/dckp_ienum_solver.hpp>
#include <dckp_ienum/ldckp_solver.hpp>
#include <dckp_ienum/types.hpp>

namespace dckp_ienum {

struct CompactSolution {
    std::vector<bool> x;

public:
    auto begin() { return x.begin(); }
    auto end() { return x.end(); }

    auto begin() const { return x.begin(); }
    auto end() const { return x.end(); }

    auto back() const { return x.back(); }
    auto back() { return x.back(); }

    bool operator[](item_index_t i) const {
        if (i < x.size()) {
            return x[i];
        } else {
            return false;
        }
    }

    friend std::ostream& operator<<(std::ostream& os, const CompactSolution& soln) {
        for (bool xi : soln.x) {
            os << (xi? '1' : '0');
        }
        return os << ']';
    }
};

struct Node {
    CompactSolution x;
    std::vector<item_index_t> conflict_set;
    int_profit_t profit = 0;
    int_weight_t weight = 0;

    Node() {}

    Node(const Node& parent, item_index_t j, int_profit_t profit, int_weight_t weight, std::vector<item_index_t> jth_conflict_set)
        : profit(profit),
          weight(weight)
    {
        // Extend solution
        x.x.reserve(j + 1);
        x.x = parent.x.x;
        x.x.resize(j + 1);
        x.x[j] = true;
        
        /* Merge conflict sets of parent and jth item */

        auto prit = parent.conflict_set.begin();
        auto prend = parent.conflict_set.end();
        
        // Skip any item in the parent which is <= j
        while (prit != prend && *prit <= j) {
            ++prit;
        }
        
        auto jrit = jth_conflict_set.begin();
        auto jrend = jth_conflict_set.end();
        conflict_set.reserve(parent.conflict_set.size() + jth_conflict_set.size());
        while (prit != prend && jrit != jrend) {
            if (*prit < *jrit) {
                conflict_set.push_back(*prit);
                ++prit;
            } else if (*jrit < *prit) {
                conflict_set.push_back(*jrit);
                ++jrit;
            } else { // equal, take one
                conflict_set.push_back(*prit);
                ++prit;
                ++jrit;
            }
        }
        std::copy(jrit, jrend, std::back_inserter(conflict_set));
        std::copy(prit, prend, std::back_inserter(conflict_set));
    }

    friend std::ostream& operator<<(std::ostream& os, const Node& node) {
        os << "p=" << node.profit << ", w=" << node.weight << ", " << node.x;
        return os;
    }
};

/// @return Whether a is a subset of b
bool is_subset(const std::vector<item_index_t>& a, const std::vector<item_index_t>& b) {
    // profiler::ScopedTicToc tictoc("is_subset");

    item_index_t a_i = 0;
    item_index_t b_i = 0;

    while (a_i < a.size() && b_i < b.size()) {
        if (a[a_i] == b[b_i]) {
            // item in A found, move to the next one
            ++a_i;
            ++b_i;
        }  else if (b[b_i] < a[a_i]) {
            // item in B is not in A, skip
            ++b_i;
        } else {
            // item in A is not in B
            return false;
        }
    }

    return a_i == a.size();
}

void solve_dckp_ienum(const dckp_ienum::Instance& instance, int_profit_t lb, Solution& soln) {
    profiler::ScopedTicToc ticto("solve_dckp_ienum");

    std::vector<Node> next_fifo;
    std::vector<Node> current_fifo;

    std::vector<item_index_t> jth_conflict_set;
    std::size_t jth_conflict_index = 0;

    // Push a node with no choices made
    current_fifo.emplace_back();
    
    for (item_index_t j = 0; j < instance.num_items(); ++j) {
        std::cout << "level " << j << ", " << current_fifo.size() << " nodes" << std::endl;

        /* Termination of unfeasible problems */
        if (current_fifo.empty()) {
            std::cout << "unfeasible" << std::endl;
            // Unfeasible
            soln.x.setZero();
            soln.p = 0;
            soln.w = 0;
            return;
        }

        /* Update jth_conflict_set */
        {
            profiler::ScopedTicToc tictoc("jth_conflict_set");
            jth_conflict_set.clear();
            // Forward jth_conflict_index until we reach the conflicts of j
            for (; jth_conflict_index < instance.conflicts().size(); ++jth_conflict_index) {
                if (instance.conflicts()[jth_conflict_index].i >= j) {
                    break;
                }
            }
            // Save all conflicts of j (with items not yet in the knapsack i.e. i > j)
            for (item_index_t conflict_idx = jth_conflict_index; conflict_idx < instance.conflicts().size() && instance.conflicts()[conflict_idx].i == j; ++conflict_idx) {
                item_index_t i = instance.conflicts()[conflict_idx].j;

                auto it = std::lower_bound(jth_conflict_set.begin(), jth_conflict_set.end(), i);
                if (it == jth_conflict_set.end() || *it != i) {
                    jth_conflict_set.insert(it, i);
                }
            }
        }

        for (std::size_t parent_idx = 0; parent_idx < current_fifo.size(); ++parent_idx) {
            auto& parent = current_fifo[parent_idx];

            if (parent.profit > lb) {
                // std::cout << "update lb " << lb << std::endl;
                lb = parent.profit;
            }

            /*
            C3 can only be checked when we have the full FIFO,
            but C1,C2,C4 can be checked before.

            Here we terminate the parent node if C3 is not satisfied.
            Then, we "terminate" the children nodes that do not satisfy C1,C2,C4 by not even creating them.
            */

            // Check C3 on the parent node
            if (false) {
                profiler::ScopedTicToc tictoc("c3");

                bool dominated = false;
                for (std::size_t other_node_idx = 0; other_node_idx < current_fifo.size(); ++other_node_idx) {
                    if (other_node_idx == parent_idx) {
                        continue;
                    }
    
                    auto& other_node = current_fifo[other_node_idx];
                    if (other_node.profit >= parent.profit && other_node.weight <= parent.weight) {
                        // Is the conflict set of the other node a subset of the current node?
                        dominated = is_subset(other_node.conflict_set, parent.conflict_set);
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

            // Don't add children if this is the last item - we only collect the best solution
            if (j == instance.num_items() - 1) {                
                if (parent.profit > soln.p) {
                    soln.w = parent.weight;
                    soln.p = parent.profit;
                    for (item_index_t i = 0; i < soln.x.size(); ++i) {
                        soln.x[i] = parent.x[i];
                    }
                }
                continue;
            }

            // C1,C2,C4 are satisfied by default if we don't add the j-th item.
            // In that case, the child is a copy of the parent.
            {
                profiler::ScopedTicToc tictoc("create_false_node");
                next_fifo.push_back(parent);
            }

            // Check C1,C2,C4 in the case we add the j-th item
            int_profit_t p = parent.profit + instance.profits()(j);
            int_weight_t w = parent.weight + instance.weights()(j);

            // Fast-path
            if (p > soln.ub) {
                // std::cout << "ub" << std::endl;
                continue;
            }

            // C1
            if (w > instance.capacity()) {
                // std::cout << "overweight" << std::endl;
                continue;
            }

            // C2
            {
                profiler::ScopedTicToc tictoc("C2");
                // It is enough to check whether item j is in the parent's conflict set
                auto it = std::find(parent.conflict_set.begin(), parent.conflict_set.end(), j);
                if (it != parent.conflict_set.end()) {
                    continue;
                }
            }

            // C4

            if (false) {
                LdckpResult result = solve_ldckp(instance, parent.x.x);
                if (result.profit_opt < lb) {
                    continue;
                }
            } else {
                auto fkpResult = solve_fkp(instance.profits().cast<float_t>(), parent.x.x, instance.weights(), instance.capacity());
                if (fkpResult.profit < lb) {
                    // std::cout << "LB" << std::endl;
                    continue;
                }
            }


            // Adding the j-th item results in a feasible node, create it
            {
                profiler::ScopedTicToc tictoc("create_true_node");
                next_fifo.emplace_back(parent, j, p, w, jth_conflict_set);
            }
        }

        // Swap the two FIFOs
        current_fifo.swap(next_fifo);
        next_fifo.clear();
        next_fifo.reserve(current_fifo.size() * 2);
    }
}

} // namespace dckp_ienum