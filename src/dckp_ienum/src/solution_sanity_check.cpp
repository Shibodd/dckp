#include "dckp_ienum/solution_has_conflicts.hpp"
#include "dckp_ienum/types.hpp"
#include <dckp_ienum/solution_sanity_check.hpp>
#include <stdexcept>

namespace dckp_ienum {

void solution_sanity_check(const Solution& solution, const dckp_ienum::Instance& instance) {
    int_profit_t profit = 0;
    int_weight_t weight = 0;
    for (item_index_t i = 0; i < instance.num_items(); ++i) {
        if (solution.x(i)) {
            profit += instance.profit(i);
            weight += instance.weight(i);
        }
    }

    bool has_conflicts = solution_has_conflicts(instance, solution.x);

    std::ostringstream os;

    if (profit != solution.p) {
        os << " Profit does not match!";
    }
    if (weight != solution.w) {
        os << " Weight does not match!";
    }
    if (profit > solution.ub) {
        os << " Profit is GT UB!";
    }
    if (weight > instance.capacity()) {
        os << " Weight is GT capacity!";
    }
    if (has_conflicts) {
        os << " Has conflicts!";
    }

    if (os.tellp() > 0) {
        throw std::runtime_error(os.str());
    }
}

} // namespace dckp_ienum