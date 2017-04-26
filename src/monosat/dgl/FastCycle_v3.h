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

#ifndef FAST_CYCLE_H_v3
#define FAST_CYCLE_H_v3

#include <vector>
#include <string.h>
#include <time.h>
#include "DynamicGraph.h"
#include "monosat/core/Config.h"
#include "Cycle.h"
namespace dgl {
template <typename Weight, bool directed=true, bool undirected=false>
class FastCycle_v3: public Cycle {
public:
	DynamicGraph<Weight>& g;

	int last_modification=0;
	int last_addition=0;
	int last_deletion=0;
	int history_qhead=0;

	int last_history_clear=0;

	int INF;

	std::vector<int> q;

	// std::vector<int> bad_edges;
	// std::vector<int> bad_edge_indices;

	const int reportPolarity;

	std::vector<int> seen_node;
	std::vector<int> seen_node_indices;
	std::vector<int> path;
	std::vector<char> seen_edge;
	std::vector<int> seen_edge_indices;
	std::vector<int> cc;	// connected component
	std::vector<char> cc_has_cycle;

	std::vector<int> cycle_reps;	// representative nodes of the cycles
	std::vector<int> cycle_reps_indices;

	std::vector<int> cycle;
	int cycle_ne, cycle_ns;

	bool has_undirected_cycle=false;
	bool has_directed_cycle=false;
	std::vector<int> undirected_cycle;
	std::vector<int> directed_cycle;

	void init() {
		int n = g.nodes();
		int m = g.edges();
		INF = n + 1;
		q.reserve(n);

		// bad_edges.reserve(m);
		// bad_edge_indices.resize(m);

		seen_node.resize(n);
		seen_node_indices.reserve(n);
		std::fill(seen_node.begin(), seen_node.end(), -1);
		path.resize(n);
		seen_edge.resize(m);
		seen_edge_indices.reserve(m);
		cc.resize(n);
		cc_has_cycle.resize(n);

		cycle_reps.reserve(n);
		cycle_reps_indices.resize(n);
		std::fill(cycle_reps_indices.begin(), cycle_reps_indices.end(), -1);

		cycle.reserve(n);
	}

	void setHasCycle() {
		has_undirected_cycle = true;
		has_directed_cycle = true;
	}

	void resetHasCycle() {
		has_undirected_cycle = false;
		has_directed_cycle = false;
	}

	void removeCycleRep(int node) {
		int index = cycle_reps_indices[node];
		// assert(index >= 0);
		cycle_reps[index] = cycle_reps.back();
		cycle_reps_indices[cycle_reps.back()] = index;
		cycle_reps.pop_back();
		cycle_reps_indices[node] = -1;
	}

	void addCycleRep(int node) {
		// assert(cycle_reps_indices[node] == -1);
		cycle_reps_indices[node] = cycle_reps.size();
		cycle_reps.push_back(node);
	}

	// // Bad Edges removal and addition methods are untested but are similar to cycle rep DS
	// void removeBadEdge(int edge_id) {
	// 	int index = bad_edge_indices[edge_id];
	// 	// assert(index >= 0);
	// 	bad_edges[index] = bad_edges.back();
	// 	bad_edge_indices[bad_edges.back()] = index;
	// 	bad_edges.pop_back();
	// 	bad_edge_indices[edge_id] = -1;
	// }

	// void addBadEdge(int edge_id) {
	// 	// assert(bad_edge_indices[edge_id] == -1);
	// 	bad_edge_indices[edge_id] = bad_edges.size();
	// 	bad_edges.push_back(edge_id);
	// }

	void resetSeenNodes() {
		for (int i=0; i<seen_node_indices.size(); i++) {
			seen_node[seen_node_indices[i]] = -1;
		}
		seen_node_indices.clear();
	}

	void resetSeenEdges() {
		for (int i=0; i<seen_edge_indices.size(); i++) {
			seen_edge[seen_edge_indices[i]] = false;
		}
		seen_edge_indices.clear();
	}

	void visitNode(int n, int prev) {
		// assert(seen_node[n] == -1);
		seen_node[n] = prev;
		seen_node_indices.push_back(n);
	}

	void setCC(int n, int comp) {
		cc[n] = comp;
	}

	void visitEdge(int id) {
		seen_edge[id] = true;
		seen_edge_indices.push_back(id);
	}

	void constructCycle(int ne, int ns, int incident_edge) {
		cycle.clear();
		cycle.push_back(incident_edge);

		int edge;
		int tmp;

		// height of ne
		tmp = ne;
		int hne = 0;
		while (tmp >= 0) {
			hne++;
			tmp = seen_node[tmp]; 
		}
		// height of ns
		tmp = ns;
		int hns = 0;
		while (tmp >= 0) {
			hns++;
			tmp = seen_node[tmp];
		}
		// create cycle based on heights
		int longer = ns, shorter = ne;
		int diff = hns - hne;
		if (hns < hne) {
			diff = hne - hns;
			longer = ne; shorter = ns;
		}
		while (diff--) {
			cycle.push_back(path[longer]);
			longer = seen_node[longer];
		}
		while (longer != shorter) {
			cycle.push_back(path[longer]);
			cycle.push_back(path[shorter]);
			longer = seen_node[longer];
			shorter = seen_node[shorter];
		}
		// for (int i=0; i<cycle.size(); i++) {
		// 	assert(g.edgeEnabled(cycle[i]));
		// }
	}

	void updateCycle() {
		if (cycle_reps.size()) {
			int ne = cycle_reps.back();
			auto incident = g.incident(ne, 0, true);

			resetSeenNodes();
			resetSeenEdges();
			
			// use the first cycle in cycle_reps
			q.clear();
			q.push_back(ne);
			visitNode(ne, -2);
			while (q.size()) {
				int n = q.back();
				q.pop_back();

				for (int ni=0; ni<g.nIncident(n, true); ni++) {
					incident = g.incident(n, ni, true);
					if (!g.hasEdge(incident.id) || !g.edgeEnabled(incident.id) || seen_edge[incident.id]) continue;
					if (seen_node[incident.node] != -1) {
						constructCycle(n, incident.node, incident.id);
						return;
					}
					path[incident.node] = incident.id;
					visitEdge(incident.id);
					visitNode(incident.node, n);
					q.push_back(incident.node);
				}
			}
		}
		// found no cycle
		// assert(false);
	}

	void DFS(int node, int ne, bool addition) {
		auto incident = g.incident(ne, 0, true);
		
		int component = (addition)? ne : node;

		q.clear();
		q.push_back(node);
		visitNode(node, -2);
		setCC(node, component);

		cc_has_cycle[cc[node]] = false;

		while (q.size()) {
			int n = q.back();
			q.pop_back();

			if (cycle_reps_indices[n] != -1) {
				removeCycleRep(n);
			}

			for (int ni = 0; ni<g.nIncident(n, true); ni++) {
				incident = g.incident(n, ni, true);

				if (!g.hasEdge(incident.id) || !g.edgeEnabled(incident.id)) {
					// if (addition && seen_node[incident.node] != -1 && cc[incident.node] == ne) {
						// addBadEdge(incident.id);
					// }
					continue;
				}

				if (seen_edge[incident.id]) continue;
				if (seen_node[incident.node] != -1) {
					addCycleRep(n);
					cc_has_cycle[component] = true;
					q.clear();
					return;
				}
				visitEdge(incident.id);
				visitNode(incident.node, n);
				q.push_back(incident.node);
				setCC(incident.node, component);
			}
		}
	}

	void computeCycles() {
		resetSeenNodes();
		resetSeenEdges();

		int hsize = g.historySize();

		// a chunk of these edge, ne, ns variables should be inside DFS
		for (int i=history_qhead; i<hsize; i++) {
			auto edge = g.getEdge(g.getChange(i).id);
			int ne = edge.to;
			int ns = edge.from;
			if (!g.hasEdge(edge.id)) continue;
			bool addition = true;
			if (!g.edgeEnabled(edge.id)) {	// does nothing for now since we're not using bad edges
				addition = false;
			}

			// removing an edge from an acyclic component is still acyclic
			if (!addition && !cc_has_cycle[cc[ne]]) {
				continue;
			}

			if (seen_node[ne] == -1) {
				DFS(ne, ne, addition);
			}
			if (!addition && seen_node[ns] == -1) {
				DFS(ns, ne, addition);
			}

		}
	}

public:	

	FastCycle_v3(DynamicGraph<Weight>& graph, bool _directed = true, int _reportPolarity = 0) :
			g(graph),  last_modification(-1), last_addition(-1), last_deletion(-1),
			history_qhead(0), last_history_clear(0), INF(0), reportPolarity(_reportPolarity) {
	}

	~FastCycle_v3() {
	}

	void update() {
		int history_size_change = g.historySize() - history_qhead;
		if (last_modification > 0 && g.modifications == last_modification ) {
			return;
		}
		if (last_modification<=0 || g.changed() || last_history_clear != g.historyclears) {
			init();
		}

		computeCycles();

		if (cycle_reps.size()) {
			setHasCycle();
		} else {
			resetHasCycle();
			cycle.clear();
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
		updateCycle();
		return cycle;
	}

	std::vector<int>& getDirectedCycle() {
		update();
		return cycle;
	}

};
};

#endif