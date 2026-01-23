#pragma once

#include <iostream>
#include <vector>
#include <filesystem>

#include <dckp_ienum/types.hpp>

namespace dckp_ienum {

struct InstanceParameters {
    item_index_t n;
    int_weight_t c;

    friend std::ostream& operator<<(std::ostream& os, const InstanceParameters& params) {
        return os << "n:" << params.n << "\tc:" << params.c;
    }
};

struct InstanceItem {
    item_index_t id;
    int_profit_t p;
    int_weight_t w;

    friend std::ostream& operator<<(std::ostream& os, const InstanceItem& item) {
        return os << "id:" << item.id << "\tp:" << item.p << "\tw:" << item.w;
    }
};

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
    std::vector<InstanceItem> m_items;
    std::vector<InstanceConflict> m_conflicts;
    InstanceParameters m_parameters;

public:
    void parse(const std::filesystem::path& path);
    void clear();

    auto& items() const { return m_items; }
    auto& conflicts() const { return m_conflicts; }
    auto& parameters() const { return m_parameters; }
};

} // namespace dckp_ienum