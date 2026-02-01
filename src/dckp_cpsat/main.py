from ortools.sat.python import cp_model
from . import instances
import dataclasses
import contextlib
import csv

@dataclasses.dataclass
class Result:
    instance: str
    status: str
    solver_time: float
    lb_time: float
    lb: float
    ub: float

class LbTimeSolutionCallback(cp_model.CpSolverSolutionCallback):
    def __init__(self):
        cp_model.CpSolverSolutionCallback.__init__(self)
        self._best_objective_value = 0.0
        self._best_objective_value_time = 0.0

    def on_solution_callback(self) -> None:
        if self.objective_value > self._best_objective_value:
            self._best_objective_value = self.objective_value
            self._best_objective_value_time = self.wall_time
            
    @property
    def best_objective_value_time(self) -> float:
        return self._best_objective_value_time

    @property
    def best_objective_value(self) -> float:
        return self._best_objective_value


def solve(root_path, instance_path_str, max_t_seconds):
    model = cp_model.CpModel()
    with instances.InstanceParser(root_path / instance_path_str) as reader:
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

    cbk = LbTimeSolutionCallback()

    solver = cp_model.CpSolver()
    solver.parameters.max_time_in_seconds = max_t_seconds
    status = solver.solve(model, cbk)
    
    assert(solver.objective_value == cbk.best_objective_value)
    
    return Result(
        instance = instance_path_str,
        solver_time = solver.wall_time,
        lb_time = cbk.best_objective_value_time,
        ub = solver.best_objective_bound,
        lb = solver.objective_value,
        status = {
            cp_model.CpSolverStatus.OPTIMAL : "optimal",
            cp_model.CpSolverStatus.FEASIBLE : "feasible"
        }.get(status, "fail")
    )

if __name__ == '__main__':
    import argparse
    import pathlib
    
    argparser = argparse.ArgumentParser()
    argparser.add_argument("instances_file", action="store", type=pathlib.Path)
    argparser.add_argument("-o", "--output", action="store", type=pathlib.Path)
    args = argparser.parse_args()

    with contextlib.ExitStack() as stack:
        writer = None
        output_file = None
        if args.output:
            output_file = stack.enter_context(args.output.open("wt", encoding="utf-8"))
            writer = csv.DictWriter(
                output_file,
                Result.__dataclass_fields__
            )
            writer.writeheader()

        for filename in args.instances_file.open("rt", encoding="utf-8"):
            instance_path = filename.strip()
            result = solve(args.instances_file.parent, instance_path, 30)
            if writer:
                writer.writerow(dataclasses.asdict(result))
                output_file.flush()

            print(result)
                