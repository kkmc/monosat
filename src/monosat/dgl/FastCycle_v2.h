/****************************************************************************************[Solver.h]
 The MIT License (MIT)

 Copyright (c) 2014, Sam Bayless

 Permission is hereby granted, free of charge, to any person obtaining a copy of this software and
 associated documentation files (the "Software"), to deal in the Software without restriction,
 including without limitation the rights to use, copy, modify, merge, publish, distribute,
 sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in all copies or
 substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT
 NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
 DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT
 OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 **************************************************************************************************/

/*********************************************************************************

NOTES:	core/Config.h has been commented out for stand alone testing / manipulation
	Currently all fields from the constructor have been left alone. Need to remove eg. polarity, or ones that
	aren't required.

*********************************************************************************/

#ifndef FAST_CYCLE_H_v2
#define FAST_CYCLE_H_v2

#include <vector>
#include <string.h>
#include <time.h>	// timing runtime speeds
#include "DynamicGraph.h"
// #include "core/Config.h"
#include "Cycle.h"
namespace dgl {
template <typename Weight, bool directed=true, bool undirected=false>
class FastCycle_v2: public Cycle {
public:
	struct Edge {
		int to;
		int from;
	};

	DynamicGraph<Weight>& g;

	int last_modification=0;
	int last_addition=0;
	int last_deletion=0;
	int history_qhead=0;

	int last_history_clear=0;

	int INF;

	std::vector<int> q;		// Nodes in DFS
	std::vector<int> check;
	std::vector<int> path;
	std::vector<int> undirected_cycle;
	std::vector<int> directed_cycle;

	std::vector<int> bad_edges;
	// std::vector<int> new_bad_edges;
	// std::vector<int> recovered_edges;

	const int reportPolarity;	// remove

	// Note that these vector<bool> is not contiguous in memory due to memory efficient spec...
	// Consider using plain array?
	bool* seen;
	std::vector<int> marked;
	std::vector<int> seen_indices;
	std::vector<char> edge_processed;

	bool has_undirected_cycle=false;
	bool has_directed_cycle=false;


	// TODO: optimize reserve sizes?
	void init(int history_size_change) {
		int n = g.nodes();
		int m = g.edges();
		q.reserve(n);
		bad_edges.resize(n);
		// marked.resize(n);
		seen = new bool[n];
		seen_indices.reserve(n);
		edge_processed.resize(m, 0);
		INF = g.nodes() + 1;
	}

	// Sets both undirected and directed cycle to true
	// Currently supports undirected cycles
	void setHasCycle() {
		has_undirected_cycle = true;
		has_directed_cycle = true;
	}

	// Prints the history status (fields from update)
	void printHistoryStatus() {
		printf("History Status vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv\n");

		printf("last_modification: %d\n", last_modification);
		printf("g.modifications: %d\n", g.modifications);

		printf("g.changed(): %d\n", g.changed());

		printf("last_history_clear: %d\n", last_history_clear);
		printf("g.historyclears: %d\n", g.historyclears);

		printf("history_qhead: %d\n", history_qhead);
		printf("g.historySize(): %d\n", g.historySize());

		printf("last_history_clear: %d\n", last_history_clear);
		printf("g.historyclears: %d\n", g.historyclears);

		printf("History Status ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^\n");
	}

	// Prints the changes since the last call to update
	void printChanges() {
		printf("Printing edge changes since last call to update.*************\n");
		int hsize = g.historySize();
		int x;
		int from, to;
		bool addi, remo;
		for (int i=history_qhead; i<hsize; i++) {
			x = g.getChange(i).id;
			from = g.getEdge(x).from;
			to = g.getEdge(x).to;
			addi = g.getChange(i).addition;
			remo = g.getChange(i).deletion;
			printf("Edge %d (%d,%d) has been %s.\n", x, from, to, (addi)?"added":((remo)?"deleted":"???"));
		}
		printf("****************************************************************");
	}

	void printq() {
		printf("q contains: ");
		for (int i=0; i<q.size(); i++) {
			printf(" %d,", q[i]);
		}
		printf("\n");
	}

	// Acyclicity Algorithm
	void propagator(int edge_id) {
		// Initialization
		q.clear();
		marked.clear();
		int seen_count = seen_indices.size();
		for (int i=0; i<seen_count; i++) {
			seen[seen_indices[i]] = false;
		}
		seen_indices.clear();
		// Retrieve ns and ne
		auto edge = g.getEdge(edge_id);
		int ne = edge.to;
		int ns = edge.from;
		int n;
		auto incident = g.incident(ne, 0, true);
		// Set ne as visited
		marked.push_back(ne);
		seen[ne] = true;
		seen_indices.push_back(ne);
		// Add children of ne != ns
		for (int i=0; i<g.nIncident(ne, true); i++) {
			incident = g.incident(ne, i, true);
			if (incident.node != ns && incident.node != ne && g.edgeEnabled(incident.id)) {
				q.push_back(incident.node);
				marked.push_back(incident.node);
				seen[incident.node] = true;
				seen_indices.push_back(incident.node);
			}
		}
		// First DFS used to find a cycle
		while (q.size()) {
			n = q.back();
			q.pop_back();
			for (int i=0; i<g.nIncident(n, true); i++) {
				incident = g.incident(n, i, true);
				if (!g.edgeEnabled(incident.id) || seen[incident.node]) continue;
				if (incident.node == ns) {
					setHasCycle();
				}
				q.push_back(incident.node);
				marked.push_back(incident.node);
				seen[incident.node] = true;
				seen_indices.push_back(incident.node);
			}
		}
		// Second DFS used to add bad edges
		q.push_back(ns);
		seen[ns] = true;
		while (q.size()) {
			n = q.back();
			q.pop_back();
			// Add bad edges
			for (int i=0; i<marked.size(); i++) {
				bad_edges[n] = marked[i];
			}
			for (int i=0; i<g.nIncident(n, true); i++) {
				incident = g.incident(n, i, true);
				if (!g.edgeEnabled(incident.id) ||seen[incident.node]) continue;
				q.push_back(incident.node);
				seen[incident.node] = true;
				seen_indices.push_back(incident.node);
			}
		}
	}

public:

	FastCycle_v2(DynamicGraph<Weight>& graph, bool _directed = true, int _reportPolarity = 0) :
			g(graph),  last_modification(-1), last_addition(-1), last_deletion(-1),
			history_qhead(0), last_history_clear(0), INF(0), reportPolarity(_reportPolarity) {
	}

	void update() {
		int history_size_change = g.historySize() - history_qhead;

		if (last_modification > 0 && g.modifications == last_modification ) {
			return;
		}

		if (last_modification<=0 || g.changed() || last_history_clear != g.historyclears) {
			init(history_size_change);
		}

		has_undirected_cycle=false;
		has_directed_cycle=false;

		// Update each edge change
		int hsize = g.historySize();
		int edge_id;
		auto edge = g.getChange(hsize-1);
		// Method 1: Preprocess the nodes that are added
		for (int i=history_qhead; i<hsize; i++) {
			edge = g.getChange(i);
			edge_id = edge.id;
			if (edge.addition) edge_processed[edge_id] = true;
			if (edge.deletion) edge_processed[edge_id] = false;
		}
		// Consider only edge additions
		for (int i=0; i<edge_processed.size(); i++) {
			edge_id = g.getChange(i).id;
			if (edge_processed[i]) propagator(edge_id);
		}

		last_modification = g.modifications;
		last_deletion = g.deletions;
		last_addition = g.additions;
		
		history_qhead = g.historySize();
		last_history_clear = g.historyclears;
	}

	bool hasDirectedCycle() {
		update();
		return has_directed_cycle;
	}

	bool hasUndirectedCycle() {
		update();
		return has_undirected_cycle;
	}

	std::vector<int>& getUndirectedCycle() {
		update();
		return undirected_cycle;
	}

	std::vector<int>& getDirectedCycle() {
		update();
		return directed_cycle;
	}

};
};

#endif



		/*
		// Method 2: Sam's method
		for (int i=history_qhead; i<hsize; i++) {
			edge = g.getChange(i);
			edge_id = edge.id;
			if (edge.addition) {
				if (g.edgeEnabled(edge_id) && !edge_processed[edge_id]) {
					edge_processed[edge_id] = true;
					propagator(edge_id);
					// printf("Here 1: process it\n");
				}
			} else if (edge.deletion) {
				if (!g.edgeEnabled(edge.id)) {
					// printf("Here 2: disabled\n");
					edge_processed[edge_id] = false;
				}
			}
		}
		*/




	// // Acyclicity Algorithm
	// void propagator(int edge_id) {
	// 	q.clear();
	// 	marked.clear();

	// 	// int g_size = g.nodes();
	// 	int seen_count = seen_indices.size();
	// 	for (int i=0; i<seen_count; i++) {
	// 		seen[seen_indices[i]] = false;
	// 	}
	// 	seen_indices.clear();

	// 	auto edge = g.getEdge(edge_id);
	// 	int ne = edge.to;
	// 	int ns = edge.from;
	// 	int n;

	// 	// if (seen[ns] || seen[ne]) {
	// 	// 	return;
	// 	// }

	// 	// printf("ns,ne are ... %d,%d\n", ns, ne);

	// 	auto incident = g.incident(ne, 0, true);

	// 	// First dfs traversal tries to find a cycle
	// 	// Traverse from 'child nodes of ne' != ns
	// 	int prev = ns;
	// 	q.push_back(ne);
	// 	// DFS and mark nodes
	// 	while (q.size()) {
	// 		n = q.back();
	// 		q.pop_back();
	// 		// if (seen[n]) {
	// 		if (n == ns) {
	// 			// TODO: Found a cycle, do something
	// 			has_undirected_cycle = true;
	// 			has_directed_cycle = true;
	// 			// printf("hello\n");
	// 			return;
	// 		}
	// 		marked.push_back(n);
	// 		seen[n] = true;
	// 		seen_indices.push_back(n);
	// 		// printf("current node seen: %d\n", n);
	// 		for (int i=0; i<g.nIncident(n, true); i++) {
	// 			incident = g.incident(n, i, true);
	// 			if (g.edgeEnabled(incident.id) && !seen[incident.node] && incident.node != prev) {
	// 				q.push_back(incident.node);
	// 				// printf("push back %d\n", incident.node);
	// 			}
	// 		}
	// 		prev = n;
	// 	}


	// 	// Second dfs traversal adds to bad edges
	// 	// Defn: Bad edges create a cycle when added to graph g
	// 	seen[ns] = true;
	// 	seen_indices.push_back(ns);
	// 	// Traverse from 'child nodes of ns' != ne
	// 	// for (int i=0; i<g.nIncident(ns, true); i++) {
	// 	// 	incident = g.incident(ns, i, true);
	// 	// 	if (g.edgeEnabled(incident.id) && incident.node != ns) {
	// 	// 		q.push_back(incident.node);
	// 	// 	}
	// 	// }
	// 	prev = ne;
	// 	// DFS and create bad edges
	// 	while (q.size()) {
	// 		n = q.back();
	// 		q.pop_back();
	// 		seen[n] = true;
	// 		seen_indices.push_back(n);

	// 		for (int i=0; i<marked.size(); i++) {
	// 			// bad_edges.push_back({ n, marked[i]});
	// 		}

	// 		for (int i=0; i<g.nIncident(n, true); i++) {
	// 			incident = g.incident(n, i, true);
	// 			if (g.edgeEnabled(incident.id) && !seen[incident.node] && incident.node != prev) {
	// 				q.push_back(incident.node);
	// 			}
	// 		}
	// 		prev = n;
	// 	}
	// }