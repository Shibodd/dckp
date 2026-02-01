import pandas as pd
import matplotlib.pyplot as plt
import pathlib

def check_sane_times(df):
    return (df["solver_time"] >= df["lb_time"]).all()

reference_df = None

def check_no_wrong_optimal(df):
    global reference_df
    # All "optimal" instances in DF should have the same lb as the reference instances in REF_DF if they are optimal in REF_DF
    mask = reference_df["status"] == "optimal"
    return (
        (df.loc[mask, "status"] != "optimal")
        | (df.loc[mask, "lb"] == reference_df.loc[mask, "lb"])
    ).all()

def check_sane_self_bounds(df):
    return (df["lb"] <= df["ub"]).all()

def check_sane_ref_bounds(df):
    global reference_df
    return ((df["lb"] <= reference_df["ub"]) & (df["ub"] >= reference_df["lb"])).all()

def estimated_optimality_gap(df):
    return ((df["ub"] - df["lb"]) / df["ub"])

def actual_optimality_gap(df):
    global reference_df
    return ((reference_df["ub"] - df["lb"]) / reference_df["ub"]).where(reference_df["status"] == "optimal")

def solved(df):
    return df["status"] == "optimal"

def worse_soln(df):
    global reference_df
    return df["lb"] < reference_df["lb"]
    
def better_soln(df):
    global reference_df
    return df["lb"] > reference_df["lb"]

def at_least_equivalent_soln(df):
    global reference_df
    return df["lb"] >= reference_df["lb"]

def sanity_check(df):
    assert(check_no_wrong_optimal(df))
    assert(check_sane_self_bounds(df))
    assert(check_sane_ref_bounds(df))
    assert(check_sane_times(df))

def add_density(df):
    df.loc[
        df['instance'].str.contains('KPCG_instances'),
        'density'
    ] = (
        df['instance'].str.extract(r'_([0-9.]+)$', expand=False)
    )
    df.loc[
        df['instance'].str.contains('sparse_'),
        'density'
    ] = (
        df['instance'].str.extract(r'r([0-9.]+)', expand=False)
    )

def add_scale(df):    
    df['scale'] = (
        df['instance'].str.extract(r'/[CR](\d+)/', expand=False)
        .astype('float')
        .astype('Int64')
        .fillna(0)
    )
    
def add_correlation(df):
    df.loc[
        df['instance'].str.contains('sparse_corr') | df['instance'].str.contains(r'/C\d+/'),
        'correlation'
    ] = 'correlated'

    df.loc[
        df['instance'].str.contains('sparse_rand') | df['instance'].str.contains(r'/R\d+/'),
        'correlation'
    ] = 'random'

    df['correlation'] = pd.Categorical(
        df['correlation'],
        categories=['correlated', 'random']
    )

def add_class(df):
    df['klass'] = (
        df['instance'].str.extract(r'BPPC_(\d+)', expand=False)
        .astype('float')    # allow NaN for sparse instances
        .astype('Int64')
    )

def add_solved(df):
    df['solved'] = df['status'] == "optimal"

def parse_df(path):
    df = pd.read_csv(path)
    sanity_check(df)
    add_class(df)
    add_solved(df)
    add_correlation(df)
    add_scale(df)
    add_density(df)
    df["actual_optimality_gap"] = actual_optimality_gap(df)
    df["estimated_optimality_gap"] = estimated_optimality_gap(df)
    return df

reference_df = pd.read_csv("cpsat.csv")
assert(check_sane_times(reference_df))
assert(check_sane_self_bounds(reference_df))
assert(check_sane_self_bounds(reference_df))


algos = ["cpsat", "bnb", "ienum"]
dfs = {
    algo: parse_df(pathlib.Path(algo).with_suffix(".csv"))
    for algo in algos
}

combined = (
    pd.concat(dfs, names=["Solver"])
      .reset_index(level="Solver")
)

grouped = combined.groupby(["correlation", "scale", "density", "Solver"], observed=True).agg(
    avg_solver_time=("solver_time", "mean"),
    avg_optimality_gap=("actual_optimality_gap", "mean"),
    avg_est_optimality_gap=("estimated_optimality_gap", "mean"),
    avg_lb_time=("lb_time", "mean"),
    solved=("solved", "sum")
)

# Unstack 'Solver' so that avg_solver_time and avg_optimality_gap are under each Solver
pivot_df = grouped.unstack(level="Solver")

# Get unique pairs to iterate through
pairs = grouped.index.droplevel(["density", "Solver"]).unique()

for corr, scale in pairs:
    # Select the specific data for this table
    # .xs (cross-section) pulls data for specific levels and drops them from the index
    sub_df = pivot_df.xs((corr, scale), level=("correlation", "scale")).copy()
    
    print(f"\n% --- LaTeX Table: Correlation {corr}, Scale {scale} ---")
    
    sub_df = sub_df.reindex(columns=algos, level=1)
    
    # Format numbers
    sub_df['avg_solver_time'] = sub_df['avg_solver_time'].round(2)
    sub_df['avg_lb_time'] = sub_df['avg_solver_time'].round(2)
    sub_df['avg_optimality_gap'] = sub_df['avg_optimality_gap'].apply(lambda col: (col * 1e2).round(2))
    sub_df['avg_est_optimality_gap'] = sub_df['avg_est_optimality_gap'].apply(lambda col: (col * 1e2).round(2))
    
    corr = "C" if corr == "correlated" else "R"
    
    sub_df = sub_df.rename(columns={
        "avg_solver_time": "Solve Time [s]",
        "avg_lb_time": "LB Time [s]",
        "avg_optimality_gap": "Act Opt Gap [\%]",
        "avg_est_optimality_gap": "Est Opt Gap [\%]"
    })
    
    latex_output = sub_df.to_latex(
        column_format="l" + "rr" * len(pivot_df.columns.levels[1]),
        caption=f"{corr}{scale} - exact solvers",
        label=f"tab:{corr}{scale}exact",
        escape=False,
        multicolumn_format='c',
        float_format="%.2f"
    )

    print(latex_output)