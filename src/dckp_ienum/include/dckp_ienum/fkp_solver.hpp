#include <Eigen/Dense>

#include <dckp_ienum/types.hpp>
#include <dckp_ienum/instance.hpp>

namespace dckp_ienum {

struct FkpResult {
    item_index_t fractional_idx;
    int_profit_t ub;
    int_profit_t profit;
    int_weight_t weight;

    void convert(const Instance&, Solution &soln, item_index_t jp1);
};

FkpResult solve_fkp_fast(const Instance& instance, item_index_t jp1, int_profit_t fixed_p, int_weight_t fixed_w);

} // namespace dckp_ienum
