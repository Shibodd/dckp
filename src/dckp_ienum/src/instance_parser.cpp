#include "dckp_ienum/types.hpp"
#include <stdexcept>
#include <iostream>
#include <charconv>
#include <numeric>
#include <fstream>
#include <string>
#include <regex>

#include <Eigen/Dense>

#include <dckp_ienum/instance.hpp>
#include <dckp_ienum/profiler.hpp>

namespace dckp_ienum {

namespace regexes {

static const std::regex n("^param n := (\\d+);$");
static const std::regex c("^param c := (\\d+);?$");
static const std::regex vertices_hdr("^param : V : p w :=$");
static const std::regex conflicts_hdr("^set E :=$");
static const std::regex vertex("^\\s+(\\d+)\\s+(\\d+)\\s+(\\d+)$");
static const std::regex conflict("^\\s+(\\d+)\\s+(\\d+)$");
static const std::regex semicolon("^;$");
static const std::regex empty("^$");

} // namespace regexes

template <typename... T>
static void parse_line(const std::string& line, const std::regex& regex, const std::string& message, std::smatch& matches_buf, T&... vars) {
    constexpr auto EXPECTED_MATCHES_COUNT = sizeof...(vars) + 1;
    
    if (std::regex_search(line, matches_buf, regex)) {
        auto actual_matches_count = matches_buf.size();

        if (actual_matches_count == EXPECTED_MATCHES_COUNT) {
            std::size_t idx = 1;

            ([&idx, &matches_buf](T& x){
                std::from_chars(matches_buf[idx].first.base(), matches_buf[idx].second.base(), x);
                ++idx;
            }(vars), ...);

            (void)idx;
        } else {
            std::ostringstream os;
            os << "Regex \"" << message << "\" matches " << actual_matches_count << " groups, but invocation is expecting " << EXPECTED_MATCHES_COUNT << "!";
            throw std::invalid_argument(os.str());
        }
    } else {
        std::ostringstream os;
        os << "Bad instance! Regex \"" << message << "\" could not be matched.";
        throw BadInstanceException(os.str());
    }
}

void Instance::clear() {
    m_conflicts.clear();
    m_num_items = 0;
}

#define PARSE_LINE(name, ...) parse_line(line_buffer, regexes::name, #name, match_buffer __VA_OPT__(,) __VA_ARGS__)
#define READ_PARSE_LINE(name, ...) do { \
    std::getline(in, line_buffer); \
    PARSE_LINE(name __VA_OPT__(,) __VA_ARGS__); \
} while(false);

void Instance::parse(const std::filesystem::path& path) {
    std::ifstream in(path);
    std::string line_buffer;
    std::smatch match_buffer;

    READ_PARSE_LINE(n, m_num_items)
    READ_PARSE_LINE(c, m_capacity);

    m_o2s_indices.resize(num_items());
    std::iota(m_o2s_indices.begin(), m_o2s_indices.end(), 0);

    m_s2o_indices.resize(num_items());
    std::iota(m_s2o_indices.begin(), m_s2o_indices.end(), 0);

    m_weights.resize(num_items());
    m_profits.resize(num_items());
    
    READ_PARSE_LINE(vertices_hdr);

    for (item_index_t i = 0; i < num_items(); ++i) {
        item_index_t file_id;
        READ_PARSE_LINE(vertex, file_id, m_profits(i), m_weights(i));
        if (file_id != i) {
            throw BadInstanceException("vertices are not continuous");
        }
    }
    READ_PARSE_LINE(semicolon);
    READ_PARSE_LINE(empty);
    READ_PARSE_LINE(conflicts_hdr);

    while (true) {
        std::getline(in, line_buffer);
        
        try {
            // List ends on the semicolon line
            PARSE_LINE(semicolon);
            break;
        }
        catch (const BadInstanceException&) {}

        auto& new_conflict = m_conflicts.emplace_back();
        PARSE_LINE(conflict, new_conflict.i, new_conflict.j);
    }

    // Ensure leftover content consists of empty lines
    while (not in.eof()) {
        READ_PARSE_LINE(empty);
    }
}

void Instance::sort_items() {
    profiler::tic("sort_items");
    
    std::sort(m_s2o_indices.begin(), m_s2o_indices.end(), [&](item_index_t a, item_index_t b) {
        float_t pwr_a = static_cast<float_t>(m_profits(a)) / static_cast<float_t>(m_weights(a));
        float_t pwr_b = static_cast<float_t>(m_profits(b)) / static_cast<float_t>(m_weights(b));
        return pwr_a > pwr_b;
    });

    // Compute a map from the old indices to the new
    for (unsigned int i = 0; i < m_num_items; ++i) {
        m_o2s_indices(m_s2o_indices(i)) = i;
    }

    // Update the conflict map with the new indices, and normalize the conflicts (i <= j)
    for (InstanceConflict& conflict : m_conflicts) {
        item_index_t i = m_o2s_indices(conflict.i);
        item_index_t j = m_o2s_indices(conflict.j);
        
        conflict.i = std::min(i, j);
        conflict.j = std::max(i, j);
    }

    // Sort conflicts by i
    std::sort(m_conflicts.begin(), m_conflicts.end(), [](const InstanceConflict& a, const InstanceConflict& b) {
        return a.i < b.i;
    });

    // Reorder profits and weights
    decltype(m_profits) profits;
    decltype(m_weights) weights;
    profits.resizeLike(m_profits);
    weights.resizeLike(m_weights);

    for (item_index_t i = 0; i < num_items(); ++i) {
        profits(m_o2s_indices(i)) = m_profits(i);
        weights(m_o2s_indices(i)) = m_weights(i);
    }

    m_profits = profits;
    m_weights = weights;

    profiler::toc("sort_items");
}

} // namespace dckp_ienum