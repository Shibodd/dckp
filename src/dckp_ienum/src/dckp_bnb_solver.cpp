#include <dckp_ienum/fkp_solver.hpp>
#include "dckp_ienum/dckp_greedy_solver.hpp"
#include "dckp_ienum/profiler.hpp"
#include "dckp_ienum/solution_ldckp_to_dckp.hpp"
#include "dckp_ienum/solution_sanity_check.hpp"
#include <dckp_ienum/solution_print.hpp>
#include <dckp_ienum/conflicts.hpp>
#include <algorithm>
#include <deque>
#include <queue>
#include <variant>
#include <boost/pool/object_pool.hpp>

#include <boost/pool/pool_alloc.hpp>
#include <boost/container/flat_set.hpp>
#include <dckp_ienum/types.hpp>
#include <dckp_ienum/instance.hpp>
#include <dckp_ienum/ldckp_solver.hpp>

namespace dckp_ienum {

using NodeId = std::vector<bool>;
using ItemSet = boost::container::flat_set<item_index_t>;

struct Node {
    NodeId id;
    int_profit_t upper_bound;
    int_profit_t profit = 0;
    int_weight_t weight = 0;

    Node(int_profit_t ub) : upper_bound(ub) {}

    struct UpperBoundLt {
        bool operator()(const Node& a, const Node& b) const {
            return a.upper_bound < b.upper_bound;
        }
    };
};

void solve_dckp_bnb(const dckp_ienum::Instance& instance, Solution& soln) {
    profiler::ScopedTicToc tictoc("solve_dckp_bnb");

    const auto rconflicts_end = instance.rconflicts().end();
    const auto conflicts_end = instance.conflicts().end();

    Solution soln_temp;
    soln_temp.x.reserve(instance.num_items());
    soln_temp.ub = std::numeric_limits<int_profit_t>::max();

    std::vector<Node> queue;
    queue.emplace_back(soln_temp.ub);

    while (not queue.empty()) {
        // Move node out of the queue

        profiler::tic("dequeue_node");
        std::pop_heap(queue.begin(), queue.end(), Node::UpperBoundLt {});
        const Node node(std::move(queue.back()));
        queue.pop_back();
        profiler::toc("dequeue_node");

        const item_index_t j = node.id.size();
        if (j >= instance.num_items()) {
            std::cout << "leaf w/ p=" << node.profit << std::endl;

            if (node.profit > soln.p) {
                soln.ub = node.upper_bound;
                soln.w = node.weight;
                soln.p = node.profit;
                soln.x = node.id;
            }
            continue;
        }

        // Check the upper bound again in case the best profit changed
        if (node.upper_bound <= soln.p) {
            std::cout << node.upper_bound << " < " << soln.p << std::endl;
            continue;
        }

        // Find the position of j's conflicts
        auto jth_rconflicts_begin = std::lower_bound(instance.rconflicts().begin(), rconflicts_end, j, [](const InstanceConflict& conflict, item_index_t j) {
            return conflict.i < j;
        });
        auto jp1th_conflicts_begin = std::lower_bound(instance.conflicts().begin(), conflicts_end, j+1, [](const InstanceConflict& conflict, item_index_t j) {
            return conflict.i < j;
        });
        
        auto jp1th_rconflicts_begin = jth_rconflicts_begin;
        advance_conflict_iterator(j+1, jp1th_rconflicts_begin, rconflicts_end);
        
        auto eval_soln = [&](bool value) {
            auto rconflicts_it = jth_rconflicts_begin;

            soln_temp.p = node.profit;
            soln_temp.w = node.weight;

            if (value) {
                soln_temp.p += instance.profit(j);
                soln_temp.w += instance.weight(j);

                if (soln_temp.p > node.upper_bound) {
                    return;
                }

                if (soln_temp.w > instance.capacity()) {
                    return;
                }

                if (check_conflict(node.id, j, rconflicts_it, rconflicts_end)) {
                    return;
                }
            }

            // Prepare the solution vector
            soln_temp.x = node.id;
            soln_temp.x.resize(instance.num_items(), false);
            soln_temp.x[j] = value;

            #ifdef ENABLE_CHECKS
            if (not std::equal(node.id.begin(), node.id.end(), soln_temp.x.begin())) {
                throw std::runtime_error("broken id copy");
            }
            #endif // ENABLE_CHECKS

            // Compute a solution to the relaxed problem
            std::variant<LdckpResult, FkpResult> result;
            if (false) {
                result = dckp_ienum::solve_ldckp(instance, soln_temp.x, j+1, soln_temp.p, soln_temp.w, jp1th_rconflicts_begin, dckp_ienum::LdckpSolverParams {});
            } else {
                result = solve_fkp_fast(instance, j+1, soln_temp.p, soln_temp.w);
            }

            std::visit([&](auto& arg) {
                arg.ub = std::min(static_cast<int_profit_t>(arg.ub), node.upper_bound);
            }, result);


            // Is this problem at least as promising as the current best solution?
            if (soln_temp.ub <= soln.p) {
                return;
            }

            bool use_solution = true;
            if (use_solution) {
                // Compute a feasible solution from the relaxed one
                profiler::tic("soln_convert");

                std::visit([&](auto& arg) {
                    arg.convert(instance, soln_temp, j+1);
                }, result);

                #ifdef ENABLE_CHECKS
                std::cout << "convert check" << std::endl;
                dckp_ienum::solution_sanity_check(soln_temp, instance, false);
                #endif // ENABLE_CHECKS

                // Greedily drop items (idx > j) that break conflicts (drop the ones with worse p/w ratio)
                {
                    auto rconflicts_rit = instance.rconflicts().rbegin();
                    const auto rconflicts_rend = std::reverse_iterator(jth_rconflicts_begin);

                    for (item_index_t _i = instance.num_items(); _i > j + 1; --_i) {
                        item_index_t i = _i - 1;
    
                        if (not soln_temp.x[i]) {
                            continue;
                        }
    
                        advance_reverse_conflict_iterator(i, rconflicts_rit, rconflicts_rend);
                        if (rconflicts_rit == rconflicts_rend) {
                            break;
                        }
    
                        if (check_conflict(soln_temp.x, i, rconflicts_rit, rconflicts_rend)) {
                            soln_temp.x[i] = false;
                            soln_temp.p -= instance.profit(i);
                            soln_temp.w -= instance.weight(i);
                        }
                    }
                }
                profiler::toc("soln_convert");
                
                #ifdef ENABLE_CHECKS
                std::cout << "drop check" << std::endl;
                dckp_ienum::solution_sanity_check(soln_temp, instance);
                #endif // ENABLE_CHECKS
                
                // Greedily take items to improve the solution
                
                profiler::tic("soln_greedy");                
                auto ith_conflicts_it = jp1th_conflicts_begin;
                for (item_index_t i = j + 1; i < instance.num_items(); ++i) {
                    // Advance conflict iterators to ith-item conflicts
                    advance_conflict_iterator(i, rconflicts_it, rconflicts_end);
                    advance_conflict_iterator(i, ith_conflicts_it, conflicts_end);
    
                    if (soln_temp.x[i]) {
                        continue;
                    }

                    // Check conflicts with both items < i and items > i (they may be set by the LDCKP problem)
                    if (check_conflict(soln_temp.x, i, rconflicts_it, rconflicts_end) || check_conflict(soln_temp.x, i, ith_conflicts_it, conflicts_end)) {
                        continue;
                    }

                    int_weight_t w_new = soln_temp.w + instance.weight(i);
                    if (w_new > instance.capacity()) {
                        continue;
                    }

                    soln_temp.w = w_new;
                    soln_temp.p = soln_temp.p + instance.profit(i);
                    soln_temp.x[i] = true;
                }
                profiler::toc("soln_greedy");
    
                #ifdef ENABLE_CHECKS
                std::cout << "greedy check" << std::endl;
                dckp_ienum::solution_sanity_check(soln_temp, instance);
                #endif // ENABLE_CHECKS
                
                // If the solution found is better than the best, use it as new best
                if (soln_temp > soln) {
                    soln = soln_temp;
                    solution_print(std::cout, soln, instance) << "\n\n";
                }
            }

            profiler::tic("push_node");
            // Push the node to the queue
            auto& new_node = queue.emplace_back(soln_temp.ub);
            new_node.id = soln_temp.x;
            new_node.id.resize(j + 1); // TODO: change this
            new_node.weight = node.weight;
            new_node.profit = node.profit;
            if (value) {
                new_node.profit += instance.profit(j);
                new_node.weight += instance.weight(j);
            }

            std::push_heap(queue.begin(), queue.end(), Node::UpperBoundLt {});

            profiler::toc("push_node");
        };

        {
            profiler::ScopedTicToc tictoc("eval_false");
            eval_soln(false);
        }
        {
            profiler::ScopedTicToc tictoc("eval_true");
            eval_soln(true);
        }
    }
}

} // namespace dckp_ienum