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
    m_items.clear();
    m_parameters = InstanceParameters {};
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

    READ_PARSE_LINE(n, m_parameters.n);
    READ_PARSE_LINE(c, m_parameters.c);
    READ_PARSE_LINE(vertices_hdr);

    m_items.resize(m_parameters.n);
    for (item_index_t i = 0; i < m_parameters.n; ++i) {
        READ_PARSE_LINE(vertex, m_items[i].id, m_items[i].p, m_items[i].w);
        if (m_items[i].id != i) {
            throw BadInstanceException("vertices are not continuous or sorted");
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

    // Compute profit/weight ratios for each item
    Eigen::ArrayX<float_t> pws(parameters().n);
    for (unsigned int i = 0; i < parameters().n; ++i) {
        auto item = m_items.at(i);
        pws(i) = static_cast<float_t>(item.p) / static_cast<float_t>(item.w);
    }
    
    // Sort the items by descending profit/weight ratio
    std::sort(m_items.begin(), m_items.end(), InstanceItem::GreaterPWRatio {});

    // Compute a map from the old indices to the new
    m_reverse_item_map.resize(parameters().n);
    for (unsigned int i = 0; i < parameters().n; ++i) {
        m_reverse_item_map(m_items.at(i).id) = i;
    }

    // Update the conflict map with the new indices
    for (InstanceConflict& conflict : m_conflicts) {
        conflict.i = m_reverse_item_map(conflict.i);
        conflict.j = m_reverse_item_map(conflict.j);
    }

    profiler::toc("sort_items");
}

} // namespace dckp_ienum