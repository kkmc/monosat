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

#ifndef FAST_CYCLE_H_
#define FAST_CYCLE_H_

#include <vector>
#include <string.h>
#include <time.h>
#include "DynamicGraph.h"
// #include "core/Config.h"
#include "Cycle.h"
namespace dgl {
template <typename Weight, bool directed=true, bool undirected=false>
class FastCycle: public Cycle {
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

	std::vector<int> q;
	std::vector<int> check;
	std::vector<int> path;
	std::vector<int> undirected_cycle;
	std::vector<int> directed_cycle;

	std::vector<int> bad_edges;
	std::vector<int> bad_edge_indices;

	const int reportPolarity;	// remove

	// Note that these vector<bool> is not contiguous in memory due to memory efficient spec...
	// Consider using plain array?
	bool* seen;
	bool* marked;
	std::vector<int> seen_indices;
	std::vector<char> edge_processed;

	bool has_undirected_cycle=false;
	bool has_directed_cycle=false;


	// TODO: optimize reserve sizes?
	void init(int history_size_change) {
		INF = g.nodes() + 1;
	}

	// Sets both undirected and directed cycle to true
	// Currently supports undirected cycles
	void setHasCycle() {
		has_undirected_cycle = true;
		has_directed_cycle = true;
	}

	void removeBadEdge(int edge_id) {
		int index = bad_edge_indices[edge_id];
		bad_edges[index] = bad_edges.back();
		bad_edges.pop_back();
		bad_edge_indices[edge_id] = -1;
	}

	void addBadEdge(int edge_id) {
		bad_edge_indices[edge_id] = bad_edges.size();
		bad_edges.push_back(edge_id);
	}

	// Acyclicity Algorithm
	void propagator(int edge_id) {
		// Initialization
		q.clear();
		int seen_count = seen_indices.size();
		int sIndex;
		for (int i=0; i<seen_count; i++) {
			sIndex = seen_indices[i];
			marked[sIndex] = false;
			seen[sIndex] = false;
		}
		seen_indices.clear();

		// Retrieve ns and ne
		auto edge = g.getEdge(edge_id);
		int ne = edge.to;
		int ns = edge.from;
		int n;
		auto incident = g.incident(ne, 0, true);

		// Set ne as visited
		marked[ne] = true;
		seen[ne] = true;
		seen_indices.push_back(ne);

		// Add children of ne != ns
		for (int i=0; i<g.nIncident(ne, true); i++) {
			incident = g.incident(ne, i, true);
			if (incident.node != ns && g.edgeEnabled(incident.id)) {
				q.push_back(incident.node);
				marked[incident.node] = true;
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
				marked[incident.node] = true;
				seen[incident.node] = true;
				seen_indices.push_back(incident.node);
			}
		}

		// Second DFS used to add bad edges
		// q.push_back(ns);
		// seen[ns] = true;
		// while (q.size()) {
		// 	n = q.back();
		// 	q.pop_back();
		// 	for (int i=0; i<g.nIncident(n, true); i++) {
		// 		incident = g.incident(n, i, true);
		// 		if (!g.edgeEnabled(incident.id)) {
		// 			if (marked[incident.node]) {
		// 				addBadEdge(incident.id);
		// 			}
		// 		} else {
		// 			if (seen[incident.node]) continue;
		// 			q.push_back(incident.node);
		// 			seen[incident.node] = true;
		// 			seen_indices.push_back(incident.node);
		// 		}
		// 	}
		// }
	}

public:

	FastCycle(DynamicGraph<Weight>& graph, bool _directed = true, int _reportPolarity = 0) :
			g(graph),  last_modification(-1), last_addition(-1), last_deletion(-1),
			history_qhead(0), last_history_clear(0), INF(0), reportPolarity(_reportPolarity) {
			bad_edge_indices.resize(g.edges());
			int n = g.nodes();
			int m = g.edges();
			q.reserve(n);
			for (int i=0; i<m; i++) {
				bad_edge_indices[i] = -1;
			}
			seen_indices.reserve(n);
			marked = new bool[n];
			seen = new bool[n];
			edge_processed.resize(m, 0);
	}

	~FastCycle() {
		delete[] marked;
		delete[] seen;
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