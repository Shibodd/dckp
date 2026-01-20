# TSP (C) R Montemanni 24.08.2022
from ortools.sat.python import cp_model
import sys
print(sys.argv)
s = open(sys.argv[1]).read().splitlines()
n=len(s)
g = [[0 for z in range(n)]for w in range(n)]
y = [[0 for z in range(n)]for w in range(n)]
for i in range(n,):
    ss=s[i].split()
    for j in range(n): g[i][j]=int(ss[j])
model = cp_model.CpModel()
s=list()
arcs=[]
for i in range(n):
    for j in range(n):
        if i!=j:
            y[i][j]=model.NewBoolVar('y[%i %i]'%(i,j))
            s.append(g[i][j]*y[i][j])
            arcs.append([i,j,y[i][j]])
model.Minimize(sum(s))
model.AddCircuit(arcs)
    
#model.Add(y[0][11]==0)
    
solver = cp_model.CpSolver()
solver.parameters.log_search_progress=True
status = solver.Solve(model)
# Print solution
print("Arcs in the solution:")
for i in range(n):
    for j in range(n):
        if i!=j and solver.Value(y[i][j])>0.5:
            print(i,"->",j)
print(" Lower Bound: ",solver.BestObjectiveBound(),"\n Upper Bound: ",solver.ObjectiveValue(),"\n Number of Branches: ",solver.NumBranches(),"\n Wall Time: ",solver.WallTime(),"\n Solution Status: ",solver.StatusName(status))

