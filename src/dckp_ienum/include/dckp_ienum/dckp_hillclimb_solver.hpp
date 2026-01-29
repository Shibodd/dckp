#pragma once

#include <dckp_ienum/instance.hpp>

namespace dckp_ienum {

struct HillclimbStats {
    std::size_t swaps = 0;
    std::size_t adds = 0;

    friend std::ostream& operator<<(std::ostream& os, const HillclimbStats& stats) {
        return os << "swaps: " << stats.swaps << ", " << "adds: " << stats.adds;
    }
};

HillclimbStats solve_dckp_hillclimb(const dckp_ienum::Instance& instance, Solution& soln);

} // namespace dckp_ienum