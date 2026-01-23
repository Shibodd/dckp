#include <chrono>
#include <unordered_map>
#include <stdexcept>
#include <sstream>

#include <dckp_ienum/profiler.hpp>

namespace dckp_ienum {
namespace profiler {

static std::unordered_map<std::string_view, Stats> data;

inline static Stats& get_or_create_stats(std::string_view name) {
    return data.try_emplace(name, name).first->second;
}

void tic(std::string_view name) {
    auto& stats = get_or_create_stats(name);
    if (stats.last_start.has_value()) {
        std::ostringstream os;
        os << "Timer " << name << " was already started!" << std::endl;
        throw std::runtime_error(os.str());
    }

    // Setting start is the very last thing we do
    stats.last_start = std::chrono::steady_clock::now();
}
void toc(std::string_view name) {
    auto end = std::chrono::steady_clock::now();

    auto& stats = get_or_create_stats(name);
    if (not stats.last_start.has_value()) {
        std::ostringstream os;
        os << "Timer " << name << " was never started!" << std::endl;
        throw std::runtime_error(os.str());
    }

    auto start = *stats.last_start;
    stats.last_start.reset();

    auto elapsed = end - start;
    stats.last = elapsed;
    if (stats.count == 0) {
        stats.avg = elapsed;
    } else {
        stats.avg += (std::chrono::duration<double>(elapsed) - stats.avg) / static_cast<double>(stats.count + 1);
    }
    stats.max = std::max(stats.max, elapsed);
    stats.min = std::min(stats.min, elapsed);
    ++stats.count;
}
const Stats& stats(std::string_view name) {
    return data.at(name);
}

void print_stats(std::ostream& os) {
    os << "\nPROFILER STATISTICS\n-----";
    for (auto kvp : data) {
        os << "-----\n" << kvp.second << "\n-----";
    }
    os << "-----\n\n";
}

} // namespace profiler
} // namespace dckp_ienum
