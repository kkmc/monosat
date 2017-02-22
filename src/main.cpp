#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <set>
#include <map>
#include <vector>
#include "monosat/dgl/DynamicGraph.h"
#include "monosat/dgl/Cycle.h"

#include "monosat/dgl/FastCycle.h"
// #include "monosat/dgl/FastCycle_v2.h"
// #include "monosat/dgl/FastCycle_vb1.h"
#include "monosat/dgl/DFSCycle.h"

using namespace std;
using namespace dgl;

void generateSpanningTree(DynamicGraph<int>& g, int n, Cycle*& cycle);
void insertCycle(DynamicGraph<int>& g, int n, Cycle*& cycle);
void printGraph(DynamicGraph<int> &g);

void insertCycle(DynamicGraph<int>& g, int i) {
	int n = g.nodes();
	g.addEdge(i, n-1);
	g.addEdge(n-1, n-2);
	g.addEdge(n-2, n-3);
	g.addEdge(n-3, n-4);
	g.addEdge(n-4, n-5);
	g.addEdge(n-5,i);
	printf("cycle: (%d,%d,%d,%d,%d,%d)\n", i, n-1, n-2, n-3, n-4, n-5);
}

// Generates a spanning tree of size n
void generateSpanningTree(DynamicGraph<int>& g, int n, int r, bool hasCycle, Cycle*& cycle) {
	// Random seed
	srand (time(NULL));
	// Add n nodes
	g.addNodes(n+5);
	// Add random edges
	for(int i=1; i<n; i++) {
		int rnd = rand() % i;
		g.addEdge(rnd, i);
		// Insert a cycle at half saturated tree
		if (hasCycle && i == n/2) insertCycle(g, i);
		// cycle->update();
		//printf("Has undirected cycle after adding to %d: %d\n", rnd, cycle->hasUndirectedCycle());
	}
	// Remove edges
	r = min(n-1, r);
	for (int i=0; i<r; i++) {
		g.disableEdge(i);
		// cycle->update();
	}
}

int main(int argc, char* argv[]) {
	// Initialize and default n=(nodes in a graph) and r=(removed edges)
	int n = 1000;
	int r = 0;
	bool hasCycle = true;

	if (argc >= 2) n = atoi(argv[1]);
	if (argc >= 3) r = atoi(argv[2]);
	if (argc >= 4) hasCycle = atoi(argv[3]);
	
	double duration;
	clock_t start;

	DynamicGraph<int> g1;
	DynamicGraph<int> g2;
	Cycle* fastCycle = new FastCycle<int,true,true>(g1, false, 1);
	Cycle* dfsCycle = new DFSCycle<int,false,true>(g2, false, 1);
	generateSpanningTree(g1, n, r, hasCycle, fastCycle);
	generateSpanningTree(g2, n, r, hasCycle, dfsCycle);
	
	start = clock();
	printf("(FC) Has undirected cycle: %d\n", fastCycle->hasUndirectedCycle());
	// int rm = 500;
	// auto edge = g1.getEdge(rm);
	// int ne = edge.to;
	// int ns = edge.from;
	// printf("ns:%d, ne:%d\n", ns, ne);
	// g1.disableEdge(rm);
	// printf("(FC) Removed edge %d; has cycle: %d\n", rm, fastCycle->hasUndirectedCycle());
	duration = ( clock() - start ) / (double) CLOCKS_PER_SEC;
	printf("Time elapsed: %f\n", duration);

	start = clock();
	printf("(DFS) Has undirected cycle %d\n", dfsCycle->hasUndirectedCycle());
	duration = ( clock() - start ) / (double) CLOCKS_PER_SEC;
	printf("Time elapsed: %f\n", duration);
}