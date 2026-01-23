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
    std::chrono::duration<double> avg;
    std::size_t count;

    Stats(std::string_view name) :
        name(name),
        last(0),
        max(std::numeric_limits<std::chrono::nanoseconds::rep>::min()),
        min(std::numeric_limits<std::chrono::nanoseconds::rep>::max()),
        avg(std::numeric_limits<double>::signaling_NaN()),
        count(0)
    {}

    friend std::ostream& operator<<(std::ostream& os, const Stats& stats) {
        using T = std::chrono::microseconds;
        os << stats.name << ":\n";
        os << "Avg " << std::chrono::duration_cast<T>(stats.avg).count() << "us (" << stats.count << " samples)\n";
        os << "Max " << std::chrono::duration_cast<T>(stats.max).count() << "us, "
           << "Min " << std::chrono::duration_cast<T>(stats.min).count() << "us";
        return os;
    }
};

void tic(std::string_view name);
void toc(std::string_view name);
const Stats& stats(std::string_view name);
void print_stats(std::ostream& os);

} // namespace profiler
} // namespace dckp_ienum