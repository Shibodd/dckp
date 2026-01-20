from ortools.sat.python import cp_model
from . import instances

import argparse
import pathlib

argparser = argparse.ArgumentParser()
argparser.add_argument("instances")
args = argparser.parse_args()

instances_paths = (f for f in pathlib.Path(".").rglob(args.instances) if f.is_file())

for instance_path in instances_paths:
    print("\n\n####", instance_path, "####")
    print("Parsing instance and setting up the solver.")
    
    model = cp_model.CpModel()
    with instances.InstanceParser(instance_path) as reader:
        params = reader.read_parameters()

        profit_expr = 0
        weight_expr = 0
        vars = {}
        
        for item in reader.read_items():
            vars[item.i] = model.new_bool_var(f"x{item.i}")
            weight_expr = weight_expr + item.w * vars[item.i]
            profit_expr = profit_expr + item.p * vars[item.i]
            
        for conflict in reader.read_conflicts():
            model.add(vars[conflict.i] + vars[conflict.j] <= 1)
            
    model.maximize(profit_expr)
    model.add(weight_expr <= params.c)

    solver = cp_model.CpSolver()
    
    print("Solving.")
    status = solver.solve(model)

    print("Best solution reported by the solver:")
    print("Status:", status)
    print("Items:", [idx for idx, var in vars.items() if solver.boolean_value(var)])
    print("Profit:", solver.objective_value, f"(ub = {solver.best_objective_bound})")
    print(f"Weight: {solver.value(weight_expr)} (capacity {params.c})")
    print("Wall time:", solver.wall_time)