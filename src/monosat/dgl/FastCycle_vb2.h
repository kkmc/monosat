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

#ifndef FAST_CYCLE_H_vb2
#define FAST_CYCLE_H_vb2

#include <vector>
#include <list>
#include <string.h>
#include <time.h>	// timing runtime speeds
#include "DynamicGraph.h"
#include "UnionFind.h"
// #include "core/Config.h"
#include "Cycle.h"
namespace dgl {
template <typename Weight, bool directed=true, bool undirected=false>
class FastCycle_vb2: public Cycle {
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

	UnionFind* uf = NULL;

	std::vector<char> edge_processed;
	std::vector<int> q;
	bool* seen;
	bool* marked;
	std::vector<int> seen_indices;

	std::vector<int> undirected_cycle;
	std::vector<int> directed_cycle;

	const int reportPolarity;	// remove

	bool has_undirected_cycle=false;
	bool has_directed_cycle=false;


	// TODO: optimize reserve sizes?
	// TODO: Ask sam about this? can we move some of these to the constructor?
	void init(int history_size_change) {
		int n = g.nodes();
		int m = g.edges();
		INF = g.nodes() + 1;
		edge_processed.resize(m, 0);
		q.reserve(n);
		seen = new bool[n];
		marked = new bool[n];
		seen_indices.reserve(n);
		if (uf == NULL) {
			uf = new UnionFind(n);
		}
	}

	// Sets both undirected and directed cycle to true
	// Currently supports undirected cycles
	void setHasCycle() {
		has_undirected_cycle = true;
		has_directed_cycle = true;
	}

	// Union find unmerge based on g
	// Recompute spanning forest for CC
	void unmerge_components(int ns, int ne) {
		// Reset seen
		// ***************** use another method as sam mentioned for resetting
		q.clear();
		for (int i=0; i<g.nodes(); i++) {
			seen[i] = false;
		}
		auto incident = g.incident(ne, 0, true);
		int n;

		// child nodes
		uf->makeset(ne);
		for (int i=0; i<g.nIncident(ne, true); i++) {
			incident = g.incident(n, i, true);
			if (g.edgeEnabled(incident.id) && incident.node != ns) {
				seen[incident.node] = true;
				q.push_back(incident.node);
			}
		}

		while (q.size()) {
			n = q.back();
			q.pop_back();
			seen[n] = true;
			uf->makeset(n);
			uf->merge(n, ne);
			for (int i=0; i<g.nIncident(n, true); i++) {
				incident = g.incident(n, i, true);
				if (g.edgeEnabled(incident.id) && !seen[incident.node]) {
					q.push_back(incident.node);
				}
			}
		}

		if (!seen[ns]){
			q.push_back(ns);
		}
		while (q.size()) {
			n = q.back();
			q.pop_back();
			seen[n] = true;
			uf->makeset(n);
			uf->merge(n, ns);
			for (int i=0; i<g.nIncident(n, true); i++) {
				incident = g.incident(n, i, true);
				if (g.edgeEnabled(incident.id) && !seen[incident.node]) {
					q.push_back(incident.node);
				}
			}
		}
	}

	void edge_addition(int edge_id) {
		auto edge = g.getEdge(edge_id);
		int ne = edge.to;
		int ns = edge.from;
		if (uf->connected(ne, ns)) {
			setHasCycle();
		} else {
			uf->merge(ne, ns);		
		}
	}

	void edge_deletion(int edge_id) {
		auto edge = g.getEdge(edge_id);
		int ne = edge.to;
		int ns = edge.from;
		unmerge_components(ns, ne);
	}

public:


	FastCycle_vb2(DynamicGraph<Weight>& graph, bool _directed = true, int _reportPolarity = 0) :
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

		for (int i=0; i<edge_processed.size(); i++) {
			edge_processed[i] = 0;
		}

		// Update each edge change
		int hsize = g.historySize();
		int edge_id;
		auto edge = g.getChange(hsize-1);	// stub for auto? bad practice?
		// Method 1: Preprocess the nodes that are added
		for (int i=history_qhead; i<hsize; i++) {
			edge = g.getChange(i);
			edge_id = edge.id;
			if (edge.addition) {
				edge_processed[edge_id]++;
			}
			if (edge.deletion) {
				edge_processed[edge_id]--;
			}
		}
		// Consider only edge additions
		for (int i=0; i<edge_processed.size(); i++) {
			edge_id = g.getChange(i).id;
			if (edge_processed[i] == 1) {
				edge_addition(edge_id);
			} else if (edge_processed[i] == -1) {
				edge_deletion(edge_id);
			}
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