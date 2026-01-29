#include "dckp_ienum/types.hpp"
#include <optional>
#include <numeric>
#include <limits>
#include <stdexcept>
#include <thread>


#include <dckp_ienum/ldckp_solver.hpp>
#include <dckp_ienum/fkp_solver.hpp>
#include <dckp_ienum/conflicts.hpp>
#include <dckp_ienum/profiler.hpp>

#ifdef ENABLE_TELEMETRY
#include <dckp_ienum/telemetry_socket.hpp>
#endif // ENABLE_TELEMETRY


namespace dckp_ienum {

#ifdef ENABLE_TELEMETRY
struct Telemetry {
    float_t Lk;
    float_t lambda_norm;
    float_t dlambda_norm;
    std::size_t k;

    template <typename Archive>
    void serialize(Archive& ar) {
        ar(cereal::make_nvp("k", k));
        ar(cereal::make_nvp("Lk", Lk));
        ar(cereal::make_nvp("lambda_norm", lambda_norm));
        ar(cereal::make_nvp("dlambda_norm", dlambda_norm));
    }
};
#endif // ENABLE_TELEMETRY

void LdckpResult::convert(const Instance& instance, Solution &soln, item_index_t jp1) {
    profiler::ScopedTicToc tictoc("convert_ldckp");

    soln.ub = ub;

    for (item_index_t ldckp_i = 0; ldckp_i < x.size(); ++ldckp_i) {
        item_index_t item_i = ldckp_i + jp1;

        if (x(ldckp_i) == 1.0) {
            soln.x[item_i] = true;
            soln.p += instance.profit(item_i);
            soln.w += instance.weight(item_i);
        } else {
            soln.x[item_i] = false;
        }
    }
}

LdckpResult solve_ldckp(const Instance& instance, std::vector<bool> fixed_items, item_index_t jp1, int_profit_t fixed_items_p, int_weight_t fixed_items_w, ConflictConstIterator jp1th_rconflict_begin, const LdckpSolverParams& params) {
    profiler::ScopedTicToc tictoc("solve_ldckp");

    LdckpResult ans;

    const auto rconflict_end = instance.rconflicts().end();
    
    item_index_t n = instance.num_items() - jp1;
    conflict_index_t m = std::distance(jp1th_rconflict_begin, rconflict_end);
    
    auto ws = instance.weights().bottomRows(n).matrix();
    Eigen::VectorX<float_t> x(n);
    Eigen::VectorX<float_t> dlambdak(m);
    Eigen::VectorX<float_t> lambdak(m);
    Eigen::VectorX<float_t> ps(n);
    
    Eigen::ArrayX<item_index_t> indices(n);
    std::iota(indices.begin(), indices.end(), 0);
    
    lambdak.setZero();

    for (std::size_t k = 0; k < params.k_max; ++k) {
        float_t Lk = static_cast<float_t>(fixed_items_p) + lambdak.sum();

        {
            profiler::ScopedTicToc tictoc("ldckp_prep_ps");

            ps = instance.profits().bottomRows(n).cast<float_t>();

            conflict_index_t cft_idx = 0;
            for (auto it = jp1th_rconflict_begin; it != rconflict_end; ++it) {
                float_t conflict_lambda = lambdak(cft_idx++);

                auto i = it->i - jp1;
                ps(i) -= conflict_lambda;
                if (it->j >= jp1) {
                    ps(it->j - jp1) -= conflict_lambda;
                } else if (fixed_items[it->j]) {
                    Lk -= conflict_lambda;
                }
            }
        }

        // Solve FKP to compute x and value of Lagrangian
        {
            profiler::ScopedTicToc tictoc("ldckp_fkp_solver");

            int_weight_t int_weight = fixed_items_w;

            // Sort indices by profit / weight ratio
            std::sort(indices.begin(), indices.end(), [&](item_index_t a, item_index_t b) {
                return (ps(a) / static_cast<float_t>(ws(a))) > (ps(b) / static_cast<float_t>(ws(b)));
            });

            // Greedily take items
            x.setZero();
            for (item_index_t i = 0; i < n; ++i) 
            {
                const float_t p = ps(indices(i));
                const int_weight_t w = ws(indices(i));
                float_t& xi = x(indices(i));

                // If taking this item doesn't profit us, stop
                // Any item after this is even worse (we sorted them by p/w ratio)
                if (p <= static_cast<float_t>(0.0)) {
                    break;
                }

                const int_weight_t avail_c = instance.capacity() - int_weight;

                if (w <= avail_c) {
                    xi = static_cast<float_t>(1.0);
                    Lk += p;
                    int_weight += w;
                } else {
                    xi = static_cast<float_t>(avail_c) / static_cast<float_t>(w);
                    Lk += xi * p;
                    break;
                }
            }
        }

        if (Lk < ans.ub) {
            ans.x = x;
            ans.ub = Lk;
        }

        if (k >= params.k_max - 1) {
            break;
        }

        // Compute the subgradient
        {
            profiler::ScopedTicToc tictoc("ldckp_sg_calc");

            // Compute gradient of lagrangian wrt lambda in lambdak
            for (auto it = jp1th_rconflict_begin; it != rconflict_end; ++it) {
                float_t& conflict_dlambda = dlambdak(std::distance(jp1th_rconflict_begin, it));
    
                conflict_dlambda = static_cast<float_t>(1.0) - x(it->i - jp1);
                if (it->j >= jp1) {
                    conflict_dlambda -= x(it->j - jp1);
                } else if (fixed_items[it->j]) {
                    conflict_dlambda -= 1.0;
                }
            }
        }

        // Perform the projected subgradient step
        {
            profiler::ScopedTicToc tictoc("ldckp_sg_step");

            float_t alpha = params.alpha;
            lambdak -= alpha * dlambdak.normalized();
            lambdak = lambdak.cwiseMax(static_cast<float_t>(0.0));
        }

        #ifdef ENABLE_TELEMETRY
        Telemetry tel;
        
        tel.Lk = Lk;
        tel.dlambda_norm = dlambdak.norm();
        tel.lambda_norm = lambdak.norm();
        tel.k = k;

        TelemetrySocket::get().send("ldckp_solver_" + std::to_string(params.alpha), tel);
        #endif // ENABLE_TELEMETRY
    }

    return ans;
}

} // namespace dckp_ienum