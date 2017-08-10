#!/usr/bin/env python3
from monosat import *
import argparse
import random
import time

SATURATION = 10
TIME_LIMIT = 10


""" Initialize monosat arguments (taken from acyclic_undirected.py) """
def init_arguments():
	parser = argparse.ArgumentParser(description='ReachTest')

	parser.add_argument('--seed',type=int,help='Random seed',default=1)

	parser.add_argument('--decide-theories',dest='decide_theories',help='Show stats',action='store_true')
	parser.add_argument('--no-decide-theories',dest='decide_theories',help='',action='store_false')
	parser.set_defaults(decide_theories=False)

	parser.add_argument('--reach-under-cnf',dest='reach_under_cnf',help='Show stats',action='store_true')
	parser.add_argument('--no-reach-under-cnf',dest='reach_under_cnf',help='',action='store_false')
	parser.set_defaults(reach_under_cnf=False)

	parser.add_argument('--conflict-min-cut',dest='conflict_min_cut',help='Show stats',action='store_true')
	parser.add_argument('--no-conflict-min-cut',dest='conflict_min_cut',help='',action='store_false')
	parser.set_defaults(conflict_min_cut=True)

	parser.add_argument('--stats',dest='stats',help='Show stats',action='store_true')
	parser.add_argument('--no-stats',dest='stats',help='',action='store_false')
	parser.set_defaults(stats=True)

	parser.add_argument('--width',type=int,help='Width of grid',default=4)
	parser.add_argument('--height',type=int,help='Height of grid (=width if unset)',default=None)

	parser.add_argument('--constraints',type=float,help='Multiple of the edges number of random, at-most-one cardinality constraints to add (default: 0.1)',default=0.1)
	parser.add_argument('--clause-size',type=int,help='Number of edges in each random at-most-one constraint (>1)',default=2)
	#parser.add_argument('-n',type=int,help='Number of reachability constraints',default=1)

	parser.add_argument('--theory-order-vsids',dest='theory_order_vsids',help='Show stats',action='store_true')
	parser.add_argument('--no-theory-order-vsids',dest='theory_order_vsids',help='',action='store_false')
	parser.set_defaults(theory_order_vsids=False)

	parser.add_argument('--vsids-both',dest='vsids_both',help='Show stats',action='store_true')
	parser.add_argument('--no-vsids-both',dest='vsids_both',help='',action='store_false')
	parser.set_defaults(vsids_both=False)

	parser.add_argument('filename')

	args = parser.parse_args()

	if args.height is None:
		args.height = args.width
	
	return args

def monosat_config():
	monosat_config = '-verb=1 -rnd-seed=%d  %s   -lazy-maxflow-decisions %s %s %s %s '%(args.seed, '-decide-theories' if args.decide_theories else '-no-decide-theories', '  -conflict-min-cut -conflict-min-cut-maxflow ' if args.conflict_min_cut else ' ', '  -reach-underapprox-cnf ' if args.reach_under_cnf else '', '-theory-order-vsids' if  args.theory_order_vsids else '' , '-vsids-both' if args.vsids_both else '')
	return monosat_config

"""
	Generates a random graph with the following process:
		1. Initilaize a graph with N nodes
		2. Select i and j in [1,N] by uniform random.
		3. Add edge (i,j) to the graph if it doesn't exist.
		4. Repeat (2) and (3) 3 times.
		5. Add the 3 edges as positive literals from step (4) as a constraint.
"""
def generate_random_constraint_graph(N=10, num_constraints=15):
	# initialization
	random.seed()
	g = Graph()
	nodes = []
	edges = []
	edge_map = {}	# 'i j' -> edge index in edges
	constraints = {}
	# 1. add nodes
	for i in range(0, N):
		n = g.addNode()
		nodes.append(n)
	# create constraints
	for c in range(0, num_constraints):
		constraint = {}
		while (len(constraint) < 3):
			# 2. select i, j by random without duplicate, where i < j
			i = random.randint(0, N)
			j = random.randint(0, N)
			while (i == j):
				j = random.randint(0, N)
			if (j < i):
				tmp = j
				j = i
				i = tmp
			# 3. add edge to graph if it doesn't exist
			edge_key = '%d %d'%(i, j)
			if (edge_key not in edge_map):
				edge_map[edge_key] = len(edges)
				edges.append(g.addEdge(i, j))
			if (edge_map[edge_key] in constraint):
				continue
			constraint[edge_map[edge_key]] = True
		# avoid duplicates using dictionary
		c_key = " ".join(str(c) for c in constraint)
		if (c_key not in constraints):
			constraints[c_key] = constraint
	# add constraints
	for c_key in constraints:
		cs = constraints[c_key]
		cs = list(cs.keys())
		AssertOr(edges[cs[0]], edges[cs[1]], edges[cs[2]])
	
	return g

if __name__ == '__main__':
	args = init_arguments()
	print('Prepared arguments: %s\n'%args)

	monosat_config = monosat_config()
	print('Monosat configuration: %s\n'%monosat_config)

	Monosat().init(monosat_config)
	Monosat().setOutputFile(args.filename)
	monosat.PBManager().setPB(MinisatPlus())

	g = generate_random_constraint_graph(N=10000, num_constraints=6000)

	Assert(g.acyclic(False))
	
	try:
		print('Solving...')
		start = time.time()*1000
		result = Solve(time_limit_seconds=TIME_LIMIT)
		end = time.time()*1000
		print('Solve required: %ds'%(end-start))
		if result:
			print('SATISFIABLE')
		else:
			print('UNSATISFIABLE')
	except SolveException:
		print('No solution in %d'%(TIME_LIMIT))