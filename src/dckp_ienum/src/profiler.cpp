#include <chrono>
#include <functional>
#include <unordered_map>
#include <stdexcept>
#include <sstream>
#include <vector>

#include <dckp_ienum/profiler.hpp>

// #define ENABLE_PROFILING

namespace dckp_ienum {
namespace profiler {

#ifdef ENABLE_PROFILING

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
    stats.total += elapsed;
    stats.last = elapsed;
    stats.max = std::max(stats.max, elapsed);
    stats.min = std::min(stats.min, elapsed);
    ++stats.count;
}
const Stats& stats(std::string_view name) {
    return data.at(name);
}

void reset() {
    data.clear();
}

void print_stats(std::ostream& os) {
    using T = decltype(data)::value_type;
    std::vector<std::reference_wrapper<T>> v(data.begin(), data.end());

    std::sort(v.begin(), v.end(), [](auto a, auto b) {
        return a.get().second.total < b.get().second.total;
    });

    os << "\nPROFILER STATISTICS\n-----";
    for (auto kvp : v) {
        os << "-----\n" << kvp.get().second << "\n-----";
    }
    os << "-----\n\n";
}

#else

void tic(std::string_view) {}
void toc(std::string_view) {}
const Stats& stats(std::string_view) { throw std::runtime_error("Profiling is disabled."); }
void reset() {}
void print_stats(std::ostream&) {}

#endif

} // namespace profiler
} // namespace dckp_ienum
