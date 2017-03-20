#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <set>
#include <map>
#include <vector>
#include <algorithm>
#include "monosat/dgl/DynamicGraph.h"
#include "monosat/dgl/Cycle.h"

#include "monosat/dgl/FastCycle.h"
#include "monosat/dgl/FastCycle_v2.h"
#include "monosat/dgl/FastCycle_v3.h"
#include "monosat/dgl/FastCycle_vb1.h"
#include "monosat/dgl/FastCycle_vb2.h"
#include "monosat/dgl/DFSCycle.h"

using namespace std;
using namespace dgl;

const int COMPLETE_REMOVAL = 10;	// weight of deleting every edge in the graph
const int ADDITION = 45;	// weight of batch being addition
const int REMOVAL = 45;	// weight of batch being removal
const int BATCH_TYPE_SPLIT = 90;	// portion of edge change being a particular kind
const int BATCH_SIZE = 300;
const int VARIATION = 100;
const int NUM_BATCHES = 1;


void generateSpanningTree(DynamicGraph<int>& g, int n, Cycle*& cycle);
void insertCycle(DynamicGraph<int>& g, int n, int cycle_size);
void printGraph(DynamicGraph<int> &g);

void insertCycle(DynamicGraph<int>& g, int i, int cycle_size) {
	int n = g.nodes();
	if (cycle_size < 3 || i > n)  {
		printf("Request unsuccessful; no cycle has been added.\n");
		return;
	}
	g.addEdge(i, n-1);
	for (int s=1; s<cycle_size-1; s++) {
		g.addEdge(n-s, n-1-s);
	}
	g.addEdge(n+1-cycle_size, i);
}

void printEdgeEndpts(DynamicGraph<int>& g, int edge_id) {
	auto edge = g.getEdge(edge_id);
	auto incident = g.incident(edge.to, 0, true);
	int ne = edge.to;
	int ns = edge.from;
	printf("ne: %d.\nns: %d.\n", ne, ns);
}

// Generates a small fixed spanning tree
void createST3(DynamicGraph<int>& g, int n, bool hasCycle) {
	g.addNodes(n);
	g.addEdge(1,2);	// 0
	g.addEdge(2,5);	// 1
	g.addEdge(3,4);	// 2
	g.addEdge(4,5);	// 3
	g.addEdge(1,3);	// 4
	g.disableEdge(3);

	g.addEdge(6,7);	// 5
	g.addEdge(7,8);	// 6
	g.addEdge(8,6); // 7
	g.disableEdge(7);

	g.addEdge(4,6); // 8
	g.addEdge(1,6); // 9
	g.disableEdge(8);
	g.disableEdge(9);
}

// Generates a spanning tree of size n
void createST2(DynamicGraph<int>& g, int n, bool hasCycle) {
	printf("Creating ST(2)\n");
	// Create graph with n + 100 nodes; 100 reserved for cycle(s)
	g.addNodes(n + 100);
	for(int i=1; i<n; i++) {
		int rnd = rand() % i;
		g.addEdge(rnd, i);
	}
	if (hasCycle) {
		insertCycle(g, n/2, 4);
	}
}

void createST(DynamicGraph<int>& g, int n, bool hasCycle) {
	printf("Creating ST(1)\n");
	// Create graph with n + 100 nodes; 100 reserved for cycle(s)
	g.addNodes(n + 100);
	int S = n;
	int m = n*n;
	vector<char> T;
	T.reserve(n);
	int current = rand() % n;
	T[current] = true;
	S--;
	while (S > 0 && m > 0) {
		m--;
		int rnd = rand() % n;
		if (!T[rnd]) {
			g.addEdge(current, rnd);
			T[rnd] = true;
			S--;
		}
		current = rnd;
	}
	printf("Number of edges: %d\n", g.edges());
	printf("S, m: %d/%d, %d/%d\n", S, n, m, n*n);
	if (hasCycle) {
		insertCycle(g, current, 4);
	}
}

void testAlgorithmOnStaticBatch(DynamicGraph<int>& g, int n, bool hasCycle) {
	if (g.nodes() == 0) {
		createST3(g, n, hasCycle);
	}
	if (hasCycle) {
		insertCycle(g, 1, 4);
	}
}

void testAlgorithmInBatches(DynamicGraph<int>& g, int n, bool hasCycle, Cycle*& cycle) {
	if (g.nodes() == 0) {
		createST(g, n, hasCycle);
		cycle->update();
	}
	printf("Preparing batches\n");
	vector<int> edgeType[2];	// types: 1 enabled, 0 disabled
	edgeType[0].reserve(g.edges());
	edgeType[1].reserve(g.edges());
	for (int i=0; i<g.edges(); i++) {
		edgeType[1].push_back(i);
	}
	vector<vector<pair<int, bool> >> batches;	// <edge, add/remove>
	batches.resize(NUM_BATCHES);

	for (int i=0; i<NUM_BATCHES; i++) {
		int rnd = rand() % (ADDITION + REMOVAL + COMPLETE_REMOVAL);
		int batch_size_type1 = BATCH_SIZE + (rand() % VARIATION);
		int batch_size_type2 = batch_size_type1 / 10;
		if (rnd < ADDITION + REMOVAL) {
			bool addition_type_batch = (rnd < ADDITION) && (edgeType[0].size() > batch_size_type1);
			int a1 = addition_type_batch?1:0;
			int a2 = addition_type_batch?0:1;
			// 90% additions or removals
			while (batch_size_type1 && edgeType[a2].size()) {
				rnd = rand() % edgeType[a2].size();
				batches[i].push_back(make_pair(edgeType[a2][rnd], addition_type_batch));
				edgeType[a1].push_back(edgeType[a2][rnd]);
				edgeType[a2][rnd] = edgeType[a2].back();
				edgeType[a2].pop_back();
				batch_size_type1--;
			}
			// 10% removals or additions
			while (batch_size_type2 && edgeType[a1].size()) {
				rnd = rand() % edgeType[a1].size();
				batches[i].push_back(make_pair(edgeType[a1][rnd], !addition_type_batch));
				edgeType[a2].push_back(edgeType[a1][rnd]);
				edgeType[a1][rnd] = edgeType[a1].back();
				edgeType[a1].pop_back();
				batch_size_type2--;
			}
		} else {
			for (int j=0; j<edgeType[1].size(); j++) {
				batches[i].push_back(make_pair(edgeType[1][j], false));
				edgeType[0].push_back(edgeType[0][j]);
			}
			edgeType[0].clear();
		}
	}
	
	// printf("Print batch %d:\n", pn);
	// int pn = 2 % NUM_BATCHES;
	// for (int i=0; i<batches[pn].size(); i++) {
	// 	printf("Edge id: %d, type: %d\n", batches[pn][i].first, batches[pn][i].second);
	// }

	double duration;
	clock_t start;
	printf("Running batches with update()\n");
	for (int i=0; i<1; i++) {	// Run with one batch for now
		int edge_id;
		bool add;
		for (int j=0; j<batches[i].size(); j++) {
			edge_id = batches[i][j].first;
			add = batches[i][j].second;
			if (add) {
				g.enableEdge(edge_id);
			} else {
				g.disableEdge(edge_id);
			}
		}
		start = clock();
		cycle->update();
		duration = ( clock() - start ) / (double) CLOCKS_PER_SEC;
		printf("Time elapsed for update() on batch[%d]: %f\n", i, duration);
	}
}

void timeHasUndirectedCycle(Cycle* cycle1, Cycle* cycle2) {
	double duration;
	clock_t start;

	start = clock();
	printf("(FC) Has undirected cycle: %d\n", cycle1->hasUndirectedCycle());
	duration = ( clock() - start ) / (double) CLOCKS_PER_SEC;
	printf("Time elapsed: %f\n", duration);

	start = clock();
	printf("(DFS) Has undirected cycle %d\n", cycle2->hasUndirectedCycle());
	duration = ( clock() - start ) / (double) CLOCKS_PER_SEC;
	printf("Time elapsed: %f\n", duration);
}

int main(int argc, char* argv[]) {
	int n = 1000;
	bool hasCycle = false;
	bool staticTest = false;
	bool treeTest = true;
	bool batchTest = false;

	if (argc >= 2) n = atoi(argv[1]);
	if (argc >= 3) hasCycle = atoi(argv[2]);
	
	srand(time(NULL));

	if (staticTest) {
		printf("================= Static test =================\n");
		DynamicGraph<int> gS;
		Cycle* fastCycleS = new FastCycle_v3<int,false,true>(gS, false, 1);
		Cycle* dfsCycleS = new DFSCycle<int,false,true>(gS, false, 1);
		testAlgorithmOnStaticBatch(gS, n, hasCycle);
		timeHasUndirectedCycle(fastCycleS, dfsCycleS);
	}

	if (treeTest) {
		printf("================= Tree test =================\n");
		DynamicGraph<int> gT;
		createST2(gT, n, hasCycle);
		Cycle* fastCycleT = new FastCycle_v3<int,false,true>(gT, false, 1);
		Cycle* dfsCycleT = new DFSCycle<int,false,true>(gT, false, 1);
		timeHasUndirectedCycle(fastCycleT, dfsCycleT);
	}

	if (batchTest) {
		printf("================= Batch test =================\n");
		DynamicGraph<int> gB;
		createST3(gB, n, hasCycle);
		Cycle* fastCycleB = new FastCycle_v3<int,false,true>(gB, false, 1);
		Cycle* dfsCycleB = new DFSCycle<int,false,true>(gB, false, 1);
		// todo
	}
}