Solvers for the Disjunctively Constrained 0-1 Knapsack Problem (DCKP).

Developed for the final exam of the Optimization Algorithms course offered by UNIMORE.


First, put your instances in the Instances/ folder and edit the instances_list.txt file if needed. That file contains a list of paths relative to itself to each instance, one per line. The file contained in the folder is the one used in the benchmarks by the candidate.

To run the CP-SAT solver, move to the src/ folder and run `python3 -m dckp_cpsat.main INSTANCE_FILE`.

To run the other solvers, first move to the src/dckp_ienum folder.
Build the executable with `cmake -S . -B build -DCMAKE_BUILD_TYPE=Release` and `make -C build`.
Run it with `./build/dckp_ienum SOLVER INSTANCE_FILE -l`. For more info, run `./build/dckp_ienum -h`.

Note that for the non- CP-SAT folders you need the Eigen and Boost program-options dependencies.
On Ubuntu, you can `apt install libeigen3-dev libboost-program-options-dev`