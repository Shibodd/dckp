#include <Eigen/Dense>

#include <dckp_ienum/types.hpp>

namespace dckp_ienum {

struct FkpResult {
    Eigen::ArrayX<float_t> x;
    float_t profit;
    int_weight_t weight;
};

FkpResult solve_fkp(const Eigen::ArrayX<float_t>& ps, const std::vector<bool>& pinned_items, const Eigen::ArrayX<int_weight_t>& ws, const int_weight_t c);

} // namespace dckp_ienum
