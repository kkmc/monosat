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
// #include "core/Config.h"
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

	std::vector<int> bad_edges;
	std::vector<int> bad_edge_indices;

	const int reportPolarity;

	std::vector<char> seen_node;
	std::vector<int> seen_node_indices;
	std::vector<char> seen_edge;
	std::vector<int> seen_edge_indices;
	std::vector<int> cc;	// connected component
	std::vector<char> process_edge;


	bool has_undirected_cycle=false;
	bool has_directed_cycle=false;
	std::vector<int> undirected_cycle;
	std::vector<int> directed_cycle;

	void init() {
		int n = g.nodes();
		int m = g.edges();
		INF = n + 1;
		q.reserve(n);
		bad_edge_indices.resize(m);
		std::fill(bad_edge_indices.begin(), bad_edge_indices.end(), -1);
		seen_node.resize(n);
		seen_node_indices.reserve(n);
		seen_edge.resize(m);
		seen_edge_indices.reserve(m);
		cc.resize(n);
		process_edge.resize(m, 0);
	}

	void setHasCycle() {
		// printf("HAS A CYCLE!!\n");
		has_undirected_cycle = has_undirected_cycle || true;
		has_directed_cycle = has_directed_cycle || true;
	}

	void resetCycle() {
		has_undirected_cycle = false;
		has_directed_cycle = false;
	}

	void removeBadEdge(int edge_id) {
		int index = bad_edge_indices[edge_id];
		bad_edges[index] = bad_edges.back();
		bad_edges.pop_back();
		bad_edge_indices[edge_id] = -1;
	}

	void addBadEdge(int edge_id) {
		if (bad_edge_indices[edge_id] != -1) {
			return;
		}
		bad_edge_indices[edge_id] = bad_edges.size();
		bad_edges.push_back(edge_id);
		// printf("Adding bad edge %d\n", edge_id);
	}

	void resetSeenNodes() {
		for (int i=0; i<seen_node_indices.size(); i++) {
			seen_node[seen_node_indices[i]] = false;
		}
		seen_node_indices.clear();
	}

	void resetSeenEdges() {
		for (int i=0; i<seen_edge_indices.size(); i++) {
			seen_edge[seen_edge_indices[i]] = false;
		}
		seen_edge_indices.clear();
	}

	void visitNode(int n) {
		seen_node[n] = true;
		seen_node_indices.push_back(n);
	}

	void setCC(int n, int comp) {
		cc[n] = comp;
	}

	void visitEdge(int id) {
		seen_edge[id] = true;
		seen_edge_indices.push_back(id);
	}

	bool hasCycle() {
		// printf("FastCycle_v3::hasCycle()\n");
		resetSeenNodes();
		resetSeenEdges();

		int hsize = g.historySize();


		for (int i=history_qhead; i<hsize; i++) {
			auto edge = g.getEdge(g.getChange(i).id);
			int ne = edge.to;
			int ns = edge.from;
			if (!g.edgeEnabled(edge.id) || seen_node[ne]) continue;
			auto incident = g.incident(ne, 0, true);

			visitNode(ne);
			setCC(ne, ne);
			q.push_back(ne);

			while (q.size()) {
				int n = q.back();
				q.pop_back();
				for (int ni = 0; ni<g.nIncident(n, true); ni++) {
					incident = g.incident(n, ni, true);
					if (!g.edgeEnabled(incident.id)) {
						// if (seen_node[incident.node] && cc[incident.node] == ne) {
							// addBadEdge(incident.id);
						// }
						continue;
					}
					if (seen_edge[incident.id]) continue;
					if (seen_node[incident.node]) {
						return true;
					}
					visitEdge(incident.id);
					visitNode(incident.node);
					setCC(incident.node, ne);
					q.push_back(incident.node);
				}
			}
		}
		return false;
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

		has_undirected_cycle=false;
		has_directed_cycle=false;

		if (hasCycle()) {
			setHasCycle();
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