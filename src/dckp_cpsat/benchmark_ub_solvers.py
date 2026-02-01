import gurobipy as gp
from gurobipy import GRB

from . import instances

import argparse
import pathlib
import math

argparser = argparse.ArgumentParser()
argparser.add_argument("instances")
args = argparser.parse_args()

def continuous_dckp(path, gpenv, use_conflicts=True) -> gp.Model:
    model = gp.Model("continuous_dkcp", env=gpenv)
    
    with instances.InstanceParser(path) as reader:
        params = reader.read_parameters()
        
        vars = {
            item.i: {
                "var": model.addVar(
                    vtype=GRB.CONTINUOUS,
                    name=f"x{item.i}",
                    lb=0.0,
                    ub=1.0
                ),
                "profit": item.p,
                "weight": item.w
            } for item in reader.read_items()
        }
        
        if use_conflicts:
            for conflict in reader.read_conflicts():
                model.addConstr(vars[conflict.i]["var"] + vars[conflict.j]["var"] <= 1)
            
    model.addConstr(sum(var["weight"] * var["var"] for var in vars.values()) <= params.c)
    model.setObjective(sum(var["profit"] * var["var"] for var in vars.values()), GRB.MAXIMIZE)
    return model

instances_paths = (f for f in pathlib.Path(".").rglob(args.instances) if f.is_file())

import pandas as pd


df = pd.DataFrame(columns=(
    "path",
    "t_with",
    "t_without"
))


import time

try:
    with gp.Env() as gpenv:
        gpenv.setParam('OutputFlag', 0)
        
        for instance_path in instances_paths:
            print("####", instance_path, "####")
            
            with continuous_dckp(instance_path, gpenv, use_conflicts=True) as model:
                model.optimize()
                t_with = model.Runtime
                assert(model.Status == GRB.OPTIMAL)
                
                print(model.ObjVal)
            
            with continuous_dckp(instance_path, gpenv, use_conflicts=False) as model:
                model.optimize()
                
                #for var in model.getVars():
                #    print(var)
                t_without = model.Runtime
                print(model.ObjVal)
                assert(model.Status == GRB.OPTIMAL)
                
            df.loc[len(df)] = (instance_path, t_with, t_without)
            # break
except KeyboardInterrupt:
    print("Stopped.")
    
overhead = df["t_with"] / df["t_without"]

print("Max exec time with: ", int(df["t_with"].min() * 1e6), "us")
print("Min exec time with: ", int(df["t_with"].max() * 1e6), "us")

print("Max exec time without: ", int(df["t_without"].min() * 1e6), "us")
print("Min exec time without: ", int(df["t_without"].max() * 1e6), "us")

print("Max overhead: ", overhead.max())
print("Min overhead: ", overhead.min())