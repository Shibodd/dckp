#pragma once

#include <iostream>
#include <vector>
#include <filesystem>
#include <Eigen/Dense>

#include <dckp_ienum/types.hpp>

namespace dckp_ienum {

struct InstanceConflict {
    item_index_t i;
    item_index_t j;

    friend std::ostream& operator<<(std::ostream& os, const InstanceConflict& conflict) {
        return os << conflict.i << "->" << conflict.j;
    }
};

struct BadInstanceException : public std::runtime_error {
    BadInstanceException(const std::string& msg) : std::runtime_error(msg) {}
};

class Instance {
    Eigen::ArrayX<item_index_t> m_o2s_indices; // original-to-storage map
    Eigen::ArrayX<item_index_t> m_s2o_indices; // storage-to-original map

    Eigen::ArrayX<int_weight_t> m_weights;
    Eigen::ArrayX<int_profit_t> m_profits;

    int_weight_t m_capacity;
    item_index_t m_num_items;
    
    std::vector<InstanceConflict> m_conflicts;

public:
    item_index_t num_items() const { return m_num_items; }
    int_weight_t capacity() const { return m_capacity; }
    
    auto& conflicts() const { return m_conflicts; }

    auto weights() const { return m_weights.topRows(num_items()); }
    auto weight(Eigen::Index i) const { return weights()(i); }
    
    auto profits() const { return m_profits.topRows(num_items()); }
    auto profit(Eigen::Index i) const { return weights()(i); }

    auto s2o_index_map() const { return m_s2o_indices.topRows(num_items()); }
    auto o2s_index_map() const { return m_o2s_indices.topRows(num_items()); }
    auto s2o_index(item_index_t i) const { return s2o_index_map()(i); }
    auto o2s_index(item_index_t i) const { return o2s_index_map()(i); }
    
    void parse(const std::filesystem::path& path);

    void clear();
    void sort_items();
};

} // namespace dckp_ienum