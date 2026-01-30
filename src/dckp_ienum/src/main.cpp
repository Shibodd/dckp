#include "dckp_ienum/conflicts.hpp"
#include "dckp_ienum/dckp_ienum_solver.hpp"
#include <boost/program_options/options_description.hpp>
#include <filesystem>
#include <ios>
#include <iostream>
#include <chrono>
#include <fstream>
#include <thread>
#include <fenv.h>

#include <dckp_ienum/dckp_greedy_solver.hpp>
#include <dckp_ienum/solution_has_conflicts.hpp>
#include <dckp_ienum/solution_ldckp_to_dckp.hpp>
#include <dckp_ienum/dckp_relax_solver.hpp>
#include <dckp_ienum/solution_sanity_check.hpp>
#include <dckp_ienum/dckp_hillclimb_solver.hpp>
#include <dckp_ienum/dckp_bnb_solver.hpp>
#include <dckp_ienum/solution_print.hpp>
#include <dckp_ienum/ldckp_solver.hpp>
#include <dckp_ienum/instance.hpp>
#include <dckp_ienum/profiler.hpp>
#include <dckp_ienum/types.hpp>

#include <dckp_ienum/fkp_solver.hpp>

#include <limits>
#include <optional>
#include <ostream>
#include <signal.h>
#include <sstream>
#include <stdexcept>
#include <numeric>
#include <boost/program_options.hpp>

using SolutionCallback = std::function<void(const dckp_ienum::Solution&)>;
using Solver = std::function<void(const dckp_ienum::Instance&, dckp_ienum::Solution&, std::atomic<bool>*, const SolutionCallback&)>;

struct Arguments {
    const Solver* solver;
    std::filesystem::path input;
    std::filesystem::path output;
    bool list;
    std::chrono::seconds::rep timeout_s;
};

std::unordered_map<std::string, Solver> solvers {
    {
        "relax", [](const dckp_ienum::Instance& instance, dckp_ienum::Solution& soln, std::atomic<bool>* stop_token, const SolutionCallback& cbk) {
            dckp_ienum::solve_dckp_relax(instance, soln, false, stop_token, cbk);
        }
    },
    {
        "bnb", [](const dckp_ienum::Instance& instance, dckp_ienum::Solution& soln, std::atomic<bool>* stop_token, const SolutionCallback& cbk) {
            dckp_ienum::solve_dckp_bnb(instance, soln, false, stop_token, cbk);
        }
    },
    {
        "ienum", [](const dckp_ienum::Instance& instance, dckp_ienum::Solution& soln, std::atomic<bool>* stop_token, const SolutionCallback& cbk) {
            // run greedy solver to get a lower bound
            dckp_ienum::solve_dckp_greedy(instance, soln, stop_token, cbk);
            dckp_ienum::solve_dckp_ienum(instance, soln.p, soln, stop_token, cbk);
        },
    },
    {
        "hillclimb", [](const dckp_ienum::Instance& instance, dckp_ienum::Solution& soln, std::atomic<bool>* stop_token, const SolutionCallback& cbk) {
            dckp_ienum::solve_dckp_hillclimb(instance, soln, stop_token, cbk);
        }
    },
    {
        "greedy", [](const dckp_ienum::Instance& instance, dckp_ienum::Solution& soln, std::atomic<bool>* stop_token, const SolutionCallback& cbk) {
            dckp_ienum::solve_dckp_greedy(instance, soln, stop_token, cbk);
        },
    }
};

std::ostream& print_solvers(std::ostream& os) {
    os << "Available solvers:\n";
    for (auto kvp : solvers) {
        os << "- " << kvp.first << "\n";
    }
    return os;
}

std::optional<Arguments> parse_args(int argc, char* argv[]) {
    namespace po = boost::program_options;

    Arguments ans;

    std::ostringstream os;
    os << std::filesystem::path(argv[0]).filename().c_str() << " [options] solver input\nAvailable options";

    std::string solver;

    po::options_description desc(os.str());
    desc.add_options()
        ("help,h", "show this help")
        ("solver", po::value(&solver), "solver")
        ("input", po::value(&ans.input), "input file")
        ("list,l", po::bool_switch(&ans.list), "instance list mode")
        ("output,o", po::value(&ans.output), "output file")
        ("timeout,t", po::value(&ans.timeout_s)->default_value(30), "timeout");

    // Positional arguments
    po::positional_options_description pos;
    pos.add("solver", 1);
    pos.add("input", 1);

    po::variables_map vm;
    try {
        po::store(
            po::command_line_parser(argc, argv)
                .options(desc)
                .positional(pos)
                .run(),
            vm
        );
        po::notify(vm);
    } catch (const po::error& e) {
        std::cerr << e.what() << "\n\n" << desc << std::endl;
        return std::nullopt;
    }

    if (vm.count("help") || !vm.count("input") || !vm.count("solver")) {
        std::cerr << desc << '\n';
        print_solvers(std::cerr);
        std::cerr << std::endl;
        return std::nullopt;
    }

    auto solver_it = solvers.find(solver);
    if (solver_it == solvers.end()) {
        std::cerr << "Invalid solver " << solver << ". ";
        print_solvers(std::cerr);
        return std::nullopt;
    }
    ans.solver = &solver_it->second;
    
    return ans;
}


std::atomic<bool> small_red_button = false;
std::atomic<bool> big_red_button = false;

void sigint_handler(int) {
    std::cerr << "You pressed the big red button. Asking the solver to stop, please wait." << std::endl;

    big_red_button.store(true);
    small_red_button.store(true);
}

void run_instance(const Arguments& args, const std::filesystem::path& root, const std::filesystem::path& path, std::ostream& csv_os) {
    std::cout << root / path << std::endl;

    dckp_ienum::Instance instance;
    instance.parse(root / path);
    instance.sort_items();

    std::cout << "n: " << instance.num_items() << std::endl;
    std::cout << "m: " << instance.conflicts().size() << std::endl;
    std::cout << "c: " << instance.capacity() << std::endl;

    dckp_ienum::Solution solution;
    solution.p = 0;
    solution.ub = std::numeric_limits<dckp_ienum::int_profit_t>::max();
    solution.w = 0;
    solution.x.resize(instance.num_items(), false);


    std::atomic<std::chrono::steady_clock::time_point> start;
    std::atomic<bool> started = false;
    std::atomic<bool> done = false;

    small_red_button.store(false);
    std::thread t([&]() {
        while (not started.load()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }

        while (not done.load()) {
            auto elapsed = std::chrono::steady_clock::now() - start.load();

            if (elapsed > std::chrono::seconds(args.timeout_s)) {
                small_red_button.store(true);
                std::cerr << "Solver took too long, pressing the small red button." << std::endl;
                return;
            }
            
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }
    });

    start = std::chrono::steady_clock::now();

    std::chrono::nanoseconds callback_time(0);

    std::chrono::steady_clock::time_point lb_timestamp = start;
    started.store(true);
    (*args.solver)(instance, solution, &small_red_button, [&](const dckp_ienum::Solution& soln) {
        lb_timestamp = std::chrono::steady_clock::now();
        dckp_ienum::solution_print(std::cout, soln, instance) << "\n\n";
        callback_time += std::chrono::steady_clock::now() - lb_timestamp;
    });
    done.store(true);

    auto end = std::chrono::steady_clock::now();
    t.join();


    // try {
    dckp_ienum::solution_sanity_check(solution, instance);
    // } catch (const std::runtime_error& err) {
    //     std::cout << "INVALID SOLUTION!" << std::endl;
    //     solution.p = 0;
    // }

    if (solution.p > 0) {
        dckp_ienum::solution_print(std::cout, solution, instance) << std::endl;
    }
    dckp_ienum::profiler::print_stats(std::cout);
    dckp_ienum::profiler::reset();

    std::string status;
    if (solution.p == 0) {
        status = "fail";
    } else if (solution.p == solution.ub) {
        status = "optimal";
    } else {
        status = "feasible";
    }

    double solver_time = std::chrono::duration<double>((end - start.load()) - callback_time).count();
    double lb_time = std::chrono::duration<double>((lb_timestamp - start.load()) - callback_time).count();
    // instance,status,solver_time,lb_time,lb,ub
    csv_os << path.c_str() << "," << status << "," << solver_time << "," << lb_time << "," << solution.p << "," << solution.ub << std::endl;
}

int main(int argc, char* argv[]) {
    auto args = parse_args(argc, argv);
    if (not args) {
        return 1;
    }
    
    feenableexcept(FE_INVALID);
    signal(SIGINT, sigint_handler);

    if (not std::filesystem::exists(args->input)) {
        std::cerr << "Input file " << args->input << " does not exist." << std::endl;
        return 1;
    }

    std::ofstream file;
    if (not args->output.empty()) {
        file.exceptions(std::ios::badbit | std::ios::failbit);
        file.open(args->output);
    }

    std::ostream* os = args->output.empty()? &std::cout : &file;

    *os << "instance,status,solver_time,lb_time,lb,ub" << std::endl;

    if (args->list) {
        std::ifstream file(args->input);
        std::string line;

        auto root = args->input.parent_path();

        while (std::getline(file, line) && not big_red_button) {
            auto path = std::filesystem::path(line);
            
            if (not std::filesystem::exists(root / path)) {
                std::cerr << "Instance file " << path << " does not exist." << std::endl;
                return 1;
            }

            run_instance(*args, root, path, *os);
        }
    } else {
        run_instance(*args, std::filesystem::path {}, args->input, *os);
    }
}