#include <limits>
#include <numeric>

#include <dckp_ienum/fkp_solver.hpp>
#include <dckp_ienum/profiler.hpp>
#include <dckp_ienum/types.hpp>

namespace dckp_ienum {

void FkpResult::convert(const Instance&, Solution &soln, item_index_t jp1) {
    profiler::ScopedTicToc tictoc("convert_fkp");

    soln.p = profit;
    soln.w = weight;
    soln.ub = ub;

    for (item_index_t i = jp1; i < fractional_idx; ++i) {
        soln.x[i] = true;
    }
    for (item_index_t i = fractional_idx; i < soln.x.size(); ++i) {
        soln.x[i] = false;
    }
}

FkpResult solve_fkp_fast(const Instance& instance, item_index_t jp1, int_profit_t fixed_p, int_weight_t fixed_w) {
    profiler::ScopedTicToc tictoc("solve_fkp_fast");

    FkpResult ans;
    ans.weight = fixed_w;
    ans.profit = fixed_p;

    for (item_index_t i = jp1; i < instance.num_items(); ++i) {
        if (ans.weight >= instance.capacity()) {
            ans.fractional_idx = i;
            break;
        }

        int_profit_t p = instance.profit(i);
        if (p <= 0) {
            ans.fractional_idx = i;
            break;
        }
        
        int_weight_t w = instance.weight(i);
        int_weight_t new_w = ans.weight + w;

        if (new_w <= instance.capacity()) {
            ans.profit += p;
            ans.weight = new_w;
        } else {
            // Set the UB to the FKP profit
            float_t fraction = static_cast<float_t>(instance.capacity() - ans.weight) / static_cast<float_t>(w);
            ans.ub = ans.profit + static_cast<int_profit_t>(fraction * static_cast<float_t>(p));
            ans.fractional_idx = i;
            return ans;
        }
    }

    ans.ub = ans.profit;
    return ans;
}

} // namespace dckp_ienum