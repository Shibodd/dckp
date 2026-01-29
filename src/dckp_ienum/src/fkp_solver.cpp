#include <limits>
#include <numeric>

#include <dckp_ienum/fkp_solver.hpp>
#include <dckp_ienum/profiler.hpp>
#include <dckp_ienum/types.hpp>

namespace dckp_ienum {

/*std::pair<item_index_t, float_t>*/ void solve_fkp_fast(const Instance& instance, item_index_t j, Solution& soln) {
    profiler::ScopedTicToc tictoc("solve_fkp_fast");

    for (item_index_t i = j + 1; i < instance.num_items(); ++i) {
        if (soln.w == instance.capacity()) {
            break;
        }

        int_profit_t p = instance.profit(i);
        if (p <= 0) {
            break;
        }
        
        int_weight_t w = instance.weight(i);
        int_weight_t new_w = soln.w + w;

        if (new_w <= instance.capacity()) {
            soln.p += p;
            soln.w = new_w;
            soln.x[i] = true;
        } else {
            // Set the UB to the FKP profit
            float_t fraction = static_cast<float_t>(instance.capacity() - soln.w) / static_cast<float_t>(w);
            soln.ub = soln.p + static_cast<int_profit_t>(fraction * static_cast<float_t>(p));
            // return { i, fraction };
            return;
        }
    }

    soln.ub = soln.p;
    return;

    // no fraction
    // return { instance.num_items(), std::numeric_limits<float_t>::signaling_NaN() };
}

} // namespace dckp_ienum