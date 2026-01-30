cmake_minimum_required (VERSION 3.18)

project (dckp_ienum)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package (Eigen3 3.3 REQUIRED NO_MODULE)
find_package (Boost 1.40 COMPONENTS program_options REQUIRED)

# list (APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
# find_package(GUROBI REQUIRED)



add_executable (${PROJECT_NAME}
src/solution_sanity_check.cpp
src/dckp_bnb_solver.cpp
src/ldckp_solver.cpp
src/dckp_ienum_solver.cpp
src/dckp_greedy_solver.cpp
src/dckp_relax_solver.cpp
src/solution_greedy_improvement.cpp
src/dckp_hillclimb_solver.cpp
src/instance_parser.cpp
src/solution_print.cpp
src/fkp_solver.cpp
src/profiler.cpp
src/main.cpp
)
target_compile_features (${PROJECT_NAME} PRIVATE cxx_std_17)
target_compile_options (${PROJECT_NAME} PRIVATE -Wall -Werror -Wpedantic)
target_include_directories (${PROJECT_NAME} PRIVATE include)
target_link_libraries (${PROJECT_NAME} PRIVATE Eigen3::Eigen Boost::program_options)