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

#ifndef FAST_CYCLE_H_vb1
#define FAST_CYCLE_H_vb1

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
class FastCycle_vb1: public Cycle {
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
	// Spanning forest
	std::vector<int> span;

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
		span.resize(n, -1);
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

	// Union find unmerge based on g
	// Recompute spanning forest for CC
	void unmerge_components(int ns, int ne) {
		// Reset seen
		// ***************** use another method as sam mentioned
		q.clear();
		for (int i=0; i<g.nodes(); i++) {
			seen[i] = false;
			marked[i] = false;
		}
		auto incident = g.incident(ne, 0, true);
		int n;

		printf("span[%d]=%d, span[%d]=%d\n", ne, span[ne], ns, span[ns]);
		// Make ns point to ne in the spanning forest
		if (span[ne] == ns) {
			int tmp = ns;
			ns = ne;
			ne = tmp;
		}

		q.push_back(ne);
		while (q.size()) {
			// c1_size++;
			n = q.back();
			q.pop_back();
			seen[n] = true;
			marked[n] = true;
			uf->makeset(n);
			uf->merge(n, ne);
			for (int i=0; i<g.nIncident(n, true); i++) {
				incident = g.incident(n, i, true);
				if (g.edgeEnabled(incident.id) && !seen[incident.node] && incident.node != ns) {
					q.push_back(incident.node);
				}
			}
		}

		for (int i=0; i<g.nodes(); i++) {
			seen[i] = false;
		}

		int ne_link = -1;
		int ns_link = -1;

		q.push_back(ns);
		while (q.size()) {
			n = q.back();
			q.pop_back();
			span[n] = -1;
			seen[n] = true;
			uf->makeset(n);
			uf->merge(n, ns);
			for (int i=0; i<g.nIncident(n, true); i++) {
				incident = g.incident(n, i, true);
				if (g.edgeEnabled(incident.id) && marked[incident.node] == true) {
					ns_link = n;
					ne_link = incident.node;
				}
				if (g.edgeEnabled(incident.id) && !seen[incident.node] && incident.node != ne) {
					q.push_back(incident.node);
				}
			}
		}

		printf("link is: %d\n", ns_link);

		if (ns_link == -1) {
			span[ns] = -1;
			return;
		} else {
			uf->merge(ns, ne);
		}

		for (int i=0; i<g.nodes(); i++) {
			seen[i] = false;
		}

		add_to_spanning_forest(ne_link, ns_link);
		printf("span[%d]=%d\n", ns_link, span[ns_link]);
		q.push_back(ns_link);
		while (q.size()) {
			n = q.back();
			q.pop_back();
			seen[n] = true;
			for (int i=0; i<g.nIncident(n, true); i++) {
				incident = g.incident(n, i, true);
				if (g.edgeEnabled(incident.id) && !seen[incident.node] && incident.node != ne_link) {
					add_to_spanning_forest(n, incident.node);
					q.push_back(incident.node);
				}
			}
		}

		printf("span[500]=%d, span[1000]=%d, span[1004]=%d\n", span[500], span[1000], span[1004]);
	}


	// // Union find unmerge based on g
	// // Recompute spanning forest for CC
	// void unmerge_components(int ns, int ne) {
	// 	// Reset seen
	// 	// ***************** use another method as sam mentioned
	// 	q.clear();
	// 	for (int i=0; i<g.nodes(); i++) {
	// 		seen[i] = false;
	// 	}
	// 	auto incident = g.incident(ne, 0, true);
	// 	int n;

	// 	// Reset the spanning forest pointers 
	// 	span[ne] = -1;
	// 	span[ns] = -1;

	// 	// int c1_size = 0;

	// 	int original_size = uf->sz[uf->find(ne)];

	// 	q.push_back(ne);
	// 	while (q.size()) {
	// 		// c1_size++;
	// 		n = q.back();
	// 		q.pop_back();
	// 		span[n] = -1;
	// 		seen[n] = true;
	// 		uf->makeset(n);
	// 		uf->merge(n, ne);
	// 		for (int i=0; i<g.nIncident(n, true); i++) {
	// 			incident = g.incident(n, i, true);
	// 			if (g.edgeEnabled(incident.id) && !seen[incident.node] && incident.node != ns) {
	// 				// printf("adding spann forest n=%d incidnetnode=%d \n", n, incident.node);
	// 				// add_to_spanning_forest(n, incident.node);
	// 				// printf("span[%d]==%d\n", n, span[n]);
	// 				q.push_back(incident.node);
	// 			}
	// 		}
	// 	}

	// 	for (int i=0; i<g.nodes(); i++) {
	// 		seen[i] = false;
	// 	}

	// 	q.push_back(ne);
	// 	while (q.size()) {
	// 		n = q.back();
	// 		q.pop_back();
	// 		seen[n] = true;
	// 		for (int i=0; i<g.nIncident(n, true); i++) {
	// 			incident = g.incident(n, i, true);
	// 			if (g.edgeEnabled(incident.id) && !seen[incident.node] && incident.node != ns) {
	// 				add_to_spanning_forest(n, incident.node);
	// 				q.push_back(incident.node);
	// 			}
	// 		}
	// 	}

	// 	for (int i=0; i<g.nodes(); i++) {
	// 		seen[i] = false;
	// 	}

	// 	q.push_back(ns);
	// 	while (q.size()) {
	// 		n = q.back();
	// 		q.pop_back();
	// 		span[n] = -1;
	// 		seen[n] = true;
	// 		uf->makeset(n);
	// 		uf->merge(n, ns);
	// 		for (int i=0; i<g.nIncident(n, true); i++) {
	// 			incident = g.incident(n, i, true);
	// 			if (g.edgeEnabled(incident.id) && !seen[incident.node] && incident.node != ne) {
	// 				// add_to_spanning_forest(n, incident.node);
	// 				q.push_back(incident.node);
	// 			}
	// 		}
	// 	}

	// 	for (int i=0; i<g.nodes(); i++) {
	// 		seen[i] = false;
	// 	}

	// 	q.push_back(ns);
	// 	while (q.size()) {
	// 		n = q.back();
	// 		q.pop_back();
	// 		seen[n] = true;
	// 		for (int i=0; i<g.nIncident(n, true); i++) {
	// 			incident = g.incident(n, i, true);
	// 			if (g.edgeEnabled(incident.id) && !seen[incident.node] && incident.node != ne) {
	// 				add_to_spanning_forest(n, incident.node);
	// 				q.push_back(incident.node);
	// 			}
	// 		}
	// 	}

	// 	// printf("c1/2_size: %d/%d, original_size: %d\n", c1_size, c2_size, original_size);
	// 	// if (c1_size + 1 == original_size) {
	// 	// 	uf->makeset(ns);
	// 	// 	uf->merge(ns, ne);
	// 	// }

	// 	// printf("component of %d: %d\n", ns, uf->find(ns));
	// 	// printf("component of %d: %d\n", ne, uf->find(ne));
	// }

	void add_to_spanning_forest(int ne, int ns) {
		if (uf->connected(ne, ns)) {
			return;
		}
		// Add directed edge only if one node is not connected
		if (span[ne] == -1 && span[ns] != ne) {
			span[ne] = ns;
		} else if (span[ns] == -1 && span[ne] != ns) {
			span[ns] = ne;
		}
	}

	void edge_addition(int edge_id) {
		auto edge = g.getEdge(edge_id);
		int ne = edge.to;
		int ns = edge.from;
		// Merge the two edges
		if (uf->connected(ne, ns)) {
			has_undirected_cycle = true;
			has_directed_cycle = true;
		}
		add_to_spanning_forest(ne, ns);
		uf->merge(ne, ns);
	}

	void edge_deletion(int edge_id) {
		auto edge = g.getEdge(edge_id);
		int ne = edge.to;
		int ns = edge.from;
		unmerge_components(ns, ne);
	}

	int spanning_tree_depth(int n) {
		int depth = 0;
		// int k = g.nodes();
		// while (span[n] != -1 && k > 0) {

		// for (int i=0; i<g.nodes(); i++) {
		// 	seen[i] = false;
		// }

		while (span[n] != -1) {
			// if (seen[n]) {
				// return -1;
			// }
			// int k = 20;
			seen[n] = true;
			// k--;
			// printf("span[%d] = %d, k=%d\n", n, span[n], k);
			depth++;
			n = span[n];
		}

		return depth;
	}

	bool retrieve_cycle(int ne, int ns) {
		// printf("compone of ne: %d\n", uf->find(ne));
		// printf("compone of ns: %d\n", uf->find(ns));
		if (!uf->connected(ne, ns)) {
			printf("There is no cycle induced by the edge (%d, %d) because they are not in the same connected compontent!\n", ne, ns);
			return false;
		}
		int ne_depth = spanning_tree_depth(ne);
		int ns_depth = spanning_tree_depth(ns);

		if (ns_depth == -1 || ne_depth == -1) {
			printf("Not well implemented, but there's a cycle here\n");
			return true;
		}

		// printf("depths: ns, ne: (%d, %d)\n", ns_depth, ne_depth);
		int shorter, longer, sdepth, ldepth;
		if (ne_depth < ns_depth) {
			shorter = ne;
			sdepth = ne_depth;
			longer = ns;
			ldepth = ns_depth;
		} else {
			shorter = ns;
			sdepth = ns_depth;
			longer = ne;
			ldepth = ne_depth;
		}

		std::list<int> cycle;
		while (sdepth < ldepth) {
			ldepth--;
			cycle.push_back(longer);
			longer = span[longer];
		}
		while (sdepth > 0 && longer != shorter) {
			sdepth--;
			cycle.push_back(longer);
			cycle.push_front(shorter);
			longer = span[longer];
			shorter = span[shorter];
		}
		cycle.push_back(shorter);
		// Print cycle / witness
		printf("Cycle: ");
		for(std::list<int>::iterator it = cycle.begin(); it != cycle.end(); it++) {
			printf("%d, ", *it);
		}
		printf("\n");
		return true;
	}

public:


	FastCycle_vb1(DynamicGraph<Weight>& graph, bool _directed = true, int _reportPolarity = 0) :
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

				// testing
				// auto edge = g.getEdge(edge_id);
				// int ne = edge.to;
				// int ns = edge.from;
			}
		}
				// testing
				retrieve_cycle(500, 1004);

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