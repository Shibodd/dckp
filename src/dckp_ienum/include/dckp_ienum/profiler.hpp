#pragma once

#include <iostream>
#include <limits>
#include <optional>
#include <string>
#include <chrono>

namespace dckp_ienum {
namespace profiler {

struct Stats {
    std::string name;
    std::optional<std::chrono::steady_clock::time_point> last_start;
    std::chrono::nanoseconds last;
    std::chrono::nanoseconds max;
    std::chrono::nanoseconds min;
    std::chrono::nanoseconds total;
    std::size_t count;

    Stats(std::string_view name) :
        name(name),
        last(0),
        max(std::numeric_limits<std::chrono::nanoseconds::rep>::min()),
        min(std::numeric_limits<std::chrono::nanoseconds::rep>::max()),
        total(0),
        count(0)
    {}

    friend std::ostream& operator<<(std::ostream& os, const Stats& stats) {
        using T = std::chrono::microseconds;
        os << stats.name << " (" << stats.count << " samples):\n";
        os << "Avg " << std::chrono::duration_cast<T>(stats.total / std::max<decltype(stats.count)>(1, stats.count)).count() << "us, "
           << "Total " << std::chrono::duration_cast<T>(stats.total).count() << "us\n";
        os << "Max " << std::chrono::duration_cast<T>(stats.max).count() << "us, "
           << "Min " << std::chrono::duration_cast<T>(stats.min).count() << "us";
           
        return os;
    }
};


void reset();
void tic(std::string_view name);
void toc(std::string_view name);
const Stats& stats(std::string_view name);
void print_stats(std::ostream& os);

class ScopedTicToc {
    std::string_view m_name;
public:
    ScopedTicToc(std::string_view name) : m_name(name) {
        tic(name);
    }
    ~ScopedTicToc() {
        toc(m_name);
    }

    ScopedTicToc(const ScopedTicToc&) = delete;
    ScopedTicToc(ScopedTicToc&&) = delete;
    ScopedTicToc& operator=(const ScopedTicToc&) = delete;
    ScopedTicToc& operator=(ScopedTicToc&&) = delete;

};

} // namespace profiler
} // namespace dckp_ienum