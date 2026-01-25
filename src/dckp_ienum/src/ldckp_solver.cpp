#include <optional>
#include <limits>
#include <thread>

#include <dckp_ienum/ldckp_solver.hpp>
#include <dckp_ienum/fkp_solver.hpp>
#include <dckp_ienum/profiler.hpp>

#ifdef ENABLE_TELEMETRY
#include <dckp_ienum/telemetry_socket.hpp>
#endif // ENABLE_TELEMETRY


namespace dckp_ienum {

#ifdef ENABLE_TELEMETRY
struct Telemetry {
    float_t Lk;
    float_t deltaLk;
    float_t lambda_norm;
    float_t dlambda_norm;
    unsigned int k;

    template <typename Archive>
    void serialize(Archive& ar) {
        ar(cereal::make_nvp("k", k));
        ar(cereal::make_nvp("Lk", Lk));
        ar(cereal::make_nvp("deltaLk", deltaLk));
        ar(cereal::make_nvp("lambda_norm", lambda_norm));
        ar(cereal::make_nvp("dlambda_norm", dlambda_norm));
    }
};
#endif // ENABLE_TELEMETRY

LdckpResult::LdckpResult(std::size_t n, std::size_t m)  :
        x_opt(n),
        lambda_opt(m),
        profit_opt(invalid_v<float_t>),
        L_opt(invalid_v<float_t>),
        weight_opt(invalid_v<int_weight_t>)
{
    x_opt.setConstant(invalid_v<float_t>);
    lambda_opt.setConstant(invalid_v<float_t>);
}

LdckpResult solve_ldckp(const Instance& instance) {
    profiler::tic("solve_ldckp");

    const item_index_t n = instance.num_items();
    const conflict_index_t m = instance.conflicts().size();

    // Prepare result with invalid values
    // L_opt will be used to identify the best iteration
    LdckpResult result(n, m);
    result.L_opt = std::numeric_limits<float_t>::max();

    // Prepare working vectors
    Eigen::VectorX<float_t> dlambdak(m);
    dlambdak.setConstant(invalid_v<float_t>);
    Eigen::ArrayX<float_t> ps(n);
    ps.setConstant(invalid_v<float_t>);

    // Lambda starts from 0
    Eigen::VectorX<float_t> lambdak(m);
    lambdak.setZero();

    // Minimize Lagrangian by subgradient method on lambda
    std::optional<float_t> Lkm1;
    for (result.k = 0; ; ++result.k) {
        // Compute profits for fractional knapsack problem

        ps = instance.profits().cast<float_t>();
        for (conflict_index_t i = 0; i < m; ++i) {
            const InstanceConflict& conflict = instance.conflicts()[i];
            const double conflict_lambda = lambdak(i);

            ps(conflict.i) -= conflict_lambda;
            ps(conflict.j) -= conflict_lambda;
        }

        // Solve fractional knapsack problem
        const FkpResult fkp_result = solve_fkp(ps, instance.weights(), instance.capacity());
        
        // Compute value of lagrangian
        const float_t Lk = fkp_result.profit + lambdak.sum();

        // Update best solution
        if (Lk < result.L_opt) {
            result.L_opt = Lk;
            result.lambda_opt = lambdak;
            result.profit_opt = fkp_result.profit;
            result.weight_opt = fkp_result.weight;
            result.x_opt = fkp_result.x;
        }

        // Termination condition
        // TODO: try different conditions
        if (result.k >= 0) {
            break;
        }

        float_t deltaL = 0.0;
        if (Lkm1.has_value()) {
            constexpr float_t EPS = static_cast<float_t>(1e-3);

            deltaL = Lk - *Lkm1;

            if (deltaL >= -EPS) {
                break;
            }
        }
        Lkm1 = Lk;

        // Compute derivative of lagrangian wrt lambda in lambdak
        dlambdak.setConstant(invalid_v<float_t>);
        for (conflict_index_t i = 0; i < m; ++i) {
            const InstanceConflict& conflict = instance.conflicts().at(i);
            dlambdak(i) = static_cast<float_t>(1.0) - fkp_result.x(conflict.i) - fkp_result.x(conflict.j);
        }
        
        // Projected subgradient with fixed step size
        // TODO: try different step size rules
        lambdak -= static_cast<float_t>(1.0) * dlambdak;
        lambdak = lambdak.cwiseMax(static_cast<float_t>(0.0));

        #ifdef ENABLE_TELEMETRY
        Telemetry tel;
        
        tel.Lk = Lk;
        tel.deltaLk = deltaL;
        tel.dlambda_norm = dlambdak.norm();
        tel.lambda_norm = lambdak.norm();
        tel.k = result.k;

        TelemetrySocket::get().send("ldckp_solver", tel);
        #endif // ENABLE_TELEMETRY
    }

    profiler::toc("solve_ldckp");

    return result;
}


} // namespace dckp_ienum