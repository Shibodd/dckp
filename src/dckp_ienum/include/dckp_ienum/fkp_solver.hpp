#include <Eigen/Dense>

#include <dckp_ienum/types.hpp>
#include <dckp_ienum/instance.hpp>

namespace dckp_ienum {

struct FkpResult {
    Eigen::ArrayX<float_t> x;
    float_t profit;
    int_weight_t weight;

    int_profit_t binary_profit;
    int_weight_t binary_weight;
};

/*std::pair<item_index_t, float_t>*/ void solve_fkp_fast(const Instance& instance, item_index_t j, Solution& soln);

} // namespace dckp_ienum
