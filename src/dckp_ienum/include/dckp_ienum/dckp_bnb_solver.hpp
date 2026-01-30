#pragma once

#include <dckp_ienum/types.hpp>
#include <dckp_ienum/instance.hpp>

namespace dckp_ienum {

void solve_dckp_bnb(const dckp_ienum::Instance& instance, Solution& soln, bool use_ldckp, std::atomic<bool>* stop_token, const std::function<void(const Solution&)>& solution_callback);

} // namespace dckp_ienum