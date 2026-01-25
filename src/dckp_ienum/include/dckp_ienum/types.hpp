#pragma once

#include <Eigen/Dense>
#include <limits>

namespace dckp_ienum {

using float_t = double;
using item_index_t = unsigned int;
using conflict_index_t = unsigned int;
using int_weight_t = unsigned int;
using int_profit_t = unsigned int;

template <typename T>
struct Invalid;

template <>
struct Invalid<float_t> {
    static constexpr float_t value = std::numeric_limits<float_t>::signaling_NaN();
};

template <>
struct Invalid<unsigned int> {
    static constexpr unsigned int value = std::numeric_limits<unsigned int>::max();
};

template <typename T>
constexpr T invalid_v = Invalid<T>::value;


struct Solution {
    Eigen::ArrayX<bool> x;
    int_profit_t p = 0;
    int_weight_t w = 0;

    int_profit_t ub = std::numeric_limits<int_profit_t>::max();
};

} // namespace dckp_ienum
