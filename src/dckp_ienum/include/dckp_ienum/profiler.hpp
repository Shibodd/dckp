#include <string>
#include <iostream>
#include <chrono>


namespace dckp_ienum {

struct Profiler {
    Profiler(const std::string& name) : running(true), name(name), start(std::chrono::steady_clock::now()) {}
    ~Profiler() { stop(); }

    void stop() {
        if (running) {
            running = false;
            auto end = std::chrono::steady_clock::now();
            std::cout << name << ": " << std::chrono::duration_cast<std::chrono::microseconds>(end - start).count() << "us" << std::endl;
        }
    }
private:
    bool running;
    std::string name;
    std::chrono::steady_clock::time_point start;
};

} // namespace dckp_ienum
