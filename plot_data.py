import pandas as pd
import matplotlib.pyplot as plt

reference_df = pd.read_csv("cpsat.csv")

df = pd.read_csv("bnb.csv")
print(df)

def check_no_wrong_optimal(df, reference_df):
    # All "optimal" instances in DF should have the same lb as the reference instances in REF_DF if they are optimal in REF_DF
    mask = reference_df["status"] == "optimal"
    return (
        (df.loc[mask, "status"] != "optimal")
        | (df.loc[mask, "lb"] == reference_df.loc[mask, "lb"])
    ).all()

def check_sane_times(df):
    return (df["solver_time"] >= df["lb_time"]).all()

def check_sane_self_bounds(df):
    return (df["lb"] <= df["lb"]).all()

def check_sane_ref_bounds(df, reference_df):
    return (df["lb"] <= reference_df["ub"]).all() and (df["ub"] >= reference_df["lb"]).all()

def estimated_optimality_gap(df):
    return (df["ub"] - df["lb"]) / df["ub"]

def actual_optimality_gap(df, reference_df):
    return ((reference_df["ub"] - df["lb"]) / reference_df["ub"]).where(reference_df["status"] == "optimal")

def solved(df):
    return df["status"] == "optimal"

def worse_soln(df, reference_df):
    return df["lb"] < reference_df["lb"]
    
def better_soln(df, reference_df):
    return df["lb"] > reference_df["lb"]

def at_least_equivalent_soln(df, reference_df):
    return df["lb"] >= reference_df["lb"]

assert(check_no_wrong_optimal(df, reference_df))
assert(check_sane_self_bounds(df))
assert(check_sane_ref_bounds(df, reference_df))
assert(check_sane_times(df))
assert(check_sane_times(reference_df))

print(worse_soln(df, reference_df).sum())
print(better_soln(df, reference_df).sum())
print(at_least_equivalent_soln(df, reference_df).sum())

plt.plot(estimated_optimality_gap(df))
plt.plot(actual_optimality_gap(df, reference_df))
plt.show()