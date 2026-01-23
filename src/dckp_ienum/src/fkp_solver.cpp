#include <numeric>

#include <dckp_ienum/fkp_solver.hpp>
#include <dckp_ienum/profiler.hpp>
#include <dckp_ienum/types.hpp>

namespace dckp_ienum {

FkpResult solve_fkp(const Eigen::ArrayX<float_t>& ps, const Eigen::ArrayX<int_weight_t>& ws, const int_weight_t c) {
    profiler::tic("solve_fkp");

    if (ps.size() != ws.size()) {
        throw "wow gj m8";
    }

    const item_index_t n = ps.size();

    Eigen::ArrayX<item_index_t> indices(n);
    std::iota(indices.begin(), indices.end(), item_index_t { 0 });

    // Sort indices by profit / weight ratio
    Eigen::ArrayX<float_t> pws = ps / ws.cast<float_t>();
    std::sort(indices.begin(), indices.end(), [&pws](item_index_t a, item_index_t b) {
        return pws(a) > pws(b);
    });

    FkpResult ans;
    ans.x.resize(n);
    ans.x.setZero();
    ans.profit = static_cast<float_t>(0.0);
    ans.weight = 0;

    for (item_index_t i = 0; i < n; ++i) 
    {
        const float_t p = ps(indices(i));
        const int_weight_t w = ws(indices(i));
        float_t& xi = ans.x(indices(i));

        // If taking this item doesn't profit us, stop
        // Any item after this is even worse (we sorted them by p/w ratio)
        if (p <= static_cast<float_t>(0.0)) {
            break;
        }

        const int_weight_t avail_c = c - ans.weight;

        if (w <= avail_c) {
            xi = static_cast<float_t>(1.0);
            ans.profit += p;
            ans.weight += w;
        } else {
            xi = static_cast<float_t>(avail_c) / static_cast<float_t>(w);
            ans.profit += xi * p;
            ans.weight = c;
            break;
        }
    }

    profiler::toc("solve_fkp");

    return ans;
}

} // namespace dckp_ienum