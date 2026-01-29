#include "dckp_ienum/solution_has_conflicts.hpp"
#include "dckp_ienum/types.hpp"
#include <dckp_ienum/solution_sanity_check.hpp>
#include <stdexcept>

#include <dckp_ienum/solution_print.hpp>

namespace dckp_ienum {

void solution_sanity_check(const Solution& solution, const dckp_ienum::Instance& instance, bool check_conflicts) {
    int_profit_t profit = 0;
    int_weight_t weight = 0;
    for (item_index_t i = 0; i < instance.num_items(); ++i) {
        if (solution.x[i]) {
            profit += instance.profit(i);
            weight += instance.weight(i);
        }
    }

    bool has_conflicts = check_conflicts && solution_has_conflicts(instance, solution.x);

    std::ostringstream os;

    if (profit != solution.p) {
        os << " Profit does not match! (actual: " << profit << ", stored: " << solution.p << ")";
    }
    if (weight != solution.w) {
        os << " Weight does not match! (actual: " << weight << ", stored: " << solution.w << ")";
    }
    if (profit > solution.ub) {
        os << " Actual profit is GT UB! (profit: " << profit << ", ub: " << solution.ub << ")";
    }
    if (weight > instance.capacity()) {
        os << " Actual weight is GT capacity! (weight: " << weight << ", capacity: " << instance.capacity() << ")";
    }
    if (has_conflicts) {
        os << " Has conflicts!";
    }

    if (os.tellp() > 0) {
        std::cerr << '\n';
        solution_print(std::cerr, solution, instance) << '\n' << std::endl;
        throw std::runtime_error(os.str());
    }
}

} // namespace dckp_ienum