#include <dckp_ienum/conflicts.hpp>
#include <dckp_ienum/instance.hpp>
#include <dckp_ienum/types.hpp>

namespace dckp_ienum {

void solve_dckp_relax(const Instance& instance, Solution& solution, bool use_ldckp, std::atomic<bool>* stop_token, const std::function<void(const Solution&)>& solution_callback);

} // namespace dckp_ienum