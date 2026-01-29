#include <dckp_ienum/instance.hpp>
#include <iostream>

namespace dckp_ienum {

std::ostream& solution_print(std::ostream& os, const Solution& soln, const dckp_ienum::Instance& instance) {
    os << "Solution: ";
    bool comma = false;
    for (item_index_t i = 0; i < soln.x.size(); ++i) {
        if (soln.x[i]) {
            if (comma) {
                os << ", ";
            }
            os << instance.s2o_index(i);
            comma = true;
        }
    }
    os << "\np: " << soln.p << " (ub = " << soln.ub << "), w: " << soln.w;
    return os;
}

} // namespace dckp_ienum