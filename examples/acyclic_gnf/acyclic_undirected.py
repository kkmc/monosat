#!/usr/bin/env python3
from monosat import *
import argparse
from itertools import tee
import random
def pairwise(iterable):
    "s -> (s0,s1), (s1,s2), (s2, s3), ..."
    a, b = tee(iterable)
    next(b, None)
    return zip(a, b)
parser = argparse.ArgumentParser(description='ReachTest')


parser.add_argument("--seed",type=int,help="Random seed",default=1)

parser.add_argument('--decide-theories',dest='decide_theories',help="Show stats",action='store_true')
parser.add_argument('--no-decide-theories',dest='decide_theories',help="",action='store_false')
parser.set_defaults(decide_theories=False)


parser.add_argument('--reach-under-cnf',dest='reach_under_cnf',help="Show stats",action='store_true')
parser.add_argument('--no-reach-under-cnf',dest='reach_under_cnf',help="",action='store_false')
parser.set_defaults(reach_under_cnf=False)


parser.add_argument('--conflict-min-cut',dest='conflict_min_cut',help="Show stats",action='store_true')
parser.add_argument('--no-conflict-min-cut',dest='conflict_min_cut',help="",action='store_false')
parser.set_defaults(conflict_min_cut=True)

parser.add_argument('--stats',dest='stats',help="Show stats",action='store_true')
parser.add_argument('--no-stats',dest='stats',help="",action='store_false')
parser.set_defaults(stats=True)

parser.add_argument("--width",type=int,help="Width of grid",default=4)
parser.add_argument("--height",type=int,help="Height of grid (=width if unset)",default=None)

parser.add_argument("--constraints",type=float,help="Multiple of the edges number of random, at-most-one cardinality constraints to add (default: 0.1)",default=0.1)
parser.add_argument("--clause-size",type=int,help="Number of edges in each random at-most-one constraint (>1)",default=2)
#parser.add_argument("-n",type=int,help="Number of reachability constraints",default=1)

parser.add_argument('--theory-order-vsids',dest='theory_order_vsids',help="Show stats",action='store_true')
parser.add_argument('--no-theory-order-vsids',dest='theory_order_vsids',help="",action='store_false')
parser.set_defaults(theory_order_vsids=False)

parser.add_argument('--vsids-both',dest='vsids_both',help="Show stats",action='store_true')
parser.add_argument('--no-vsids-both',dest='vsids_both',help="",action='store_false')
parser.set_defaults(vsids_both=False)

parser.add_argument('filename')

args = parser.parse_args()

if args.height is None:
    args.height = args.width


if args.seed is None:
    args.seed = 1
print("Random seed: %d"%(args.seed))
random.seed(args.seed)

monosat_config = "-verb=1 -rnd-seed=%d  %s   -lazy-maxflow-decisions %s %s %s %s "%(args.seed, "-decide-theories" if args.decide_theories else "-no-decide-theories", "  -conflict-min-cut -conflict-min-cut-maxflow " if args.conflict_min_cut else " ", "  -reach-underapprox-cnf " if args.reach_under_cnf else "", "-theory-order-vsids" if  args.theory_order_vsids else "" , "-vsids-both" if args.vsids_both else "")
print("Monosat configuration: " + monosat_config)
Monosat().init(monosat_config)
Monosat().setOutputFile(args.filename)
monosat.PBManager().setPB(MinisatPlus())
g1= Graph()
g2= Graph()
grid = dict()
nodes = dict()
for x in range(args.width):
    for y in range(args.height):
        grid[(x,y)] = g1.addNode()
        g2.addNode()
        nodes[grid[(x,y)]] = (x,y)
all_edges=[]
edges = dict()
edge_map=dict()
for x in range(args.width):
    for y in range(args.height):
        n1 = (x,y)
        es = []
        if x+1 < args.width:
            n2 = (x+1,y)
            #print(str(n1) + "->" + str(n2))
            edges[(n1,n2)] = e1 = g1.addEdge(grid[n1],grid[n2])
            edge_map[e1] = g2.addEdge(grid[n1],grid[n2])
            #edges[(n2,n1)] = e1b = g.addEdge(grid[n2],grid[n1])
            all_edges.append(e1)
            es.append(e1)
        if y+1 < args.height:
            n2 = (x,y+1)


            edges[(n1,n2)] = e2 = g1.addEdge(grid[n1],grid[n2])
            #edges[(n2,n1)] = e2b = g.addEdge(grid[n2],grid[n1])
            edge_map[e2] = g2.addEdge(grid[n1],grid[n2])
            es.append(e2)
            all_edges.append(e2)
        if x-1>=0:
            n2 = (x-1,y)
            #print(str(n1) + "->" + str(n2))
            edges[(n1,n2)] = e1b = g1.addEdge(grid[n1],grid[n2])
            edge_map[e1b] = g2.addEdge(grid[n1],grid[n2])
            es.append(e1b)
            all_edges.append(e1b)
        if y-1>=0:
            n2 = (x,y-1)
            #print(str(n1) + "->" + str(n2))
            edges[(n1,n2)] = e2b = g1.addEdge(grid[n1],grid[n2])
            edge_map[e2b] = g2.addEdge(grid[n1],grid[n2])
            es.append(e2b)
            all_edges.append(e2b)

        #if(len(es)>=3):
        #AssertXor(es)


edgelist =all_edges

n_constraints = int(args.constraints * len(edgelist))
print("#nodes %d #edges %d, #constraints %d"%(g1.numNodes() ,g1.numEdges(), n_constraints))

for n in range(n_constraints):
    n1 = random.randint(0,len(edgelist)-1)
    n2 = random.randint(0,len(edgelist)-2)
    if n2 >= n1:
        n2+=1
    AssertOr(~edgelist[n1], ~edgelist[n2])
    #AssertXor(edgelist[n1], edgelist[n2])
    #AssertLessEqPB((edgelist[n1], edgelist[n2] ), 1) #At most one of these edges may be selected

#AssertGreaterEqPB(edgelist,n_constraints)

#top left node must reach bottom right node
for e in all_edges:
    e2 = edge_map[e]
    AssertXor(e,e2) #Every edge is either in the first graph, or the second, but not in both

# Assert(Not(g1.acyclic(False)))
# Assert(Not(g2.acyclic(False)))
Assert(g1.acyclic(False))
Assert(g2.acyclic(False))

#
# for n in test_model:
#
#     v = abs(n)-1
#     if(v > 0 and v<9801):
#         sn = n<0
#         #l = Var(v)
#         l = all_edges[v-1]
#         if (sn):
#             l = ~l
#
#         Assert(l)

print("Writing...")
WriteConstraints()
#print("Solving...")
#result = Solve()
# print(result)
#
# if result:
#     print("SAT")
#     sys.exit(10)
# else:
#     sys.exit(20)

