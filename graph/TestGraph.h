/*
 * SimpleGraph.h
 *
 *  Created on: 2013-04-14
 *      Author: sam
 */

#ifndef TESTGRAPH_H_
#define TESTGRAPH_H_

#include "core/Theory.h"
#include "Graph.h"
namespace Minisat{
class TestGraph:public Graph,public Theory{
private:
	int nodes;
	struct Edge{

		Lit l;
		int from;
		int to;
	};
	vec<vec<Edge> > edges;
	Lit False;
	Lit True;
	Solver * S;

public:
	TestGraph(Solver * S_):S(S_){
		True = mkLit(S->newVar(),false);
			False=~True;
			S->addClause(True);
	}
     ~TestGraph(){};
	 int newNode(){
			edges.push();
		return nodes++;
	}
	int nNodes(){

		return nodes;
	}
	bool isNode(int n){
		return n>=0 && n<nodes;
	}
	void backtrackUntil(int level){};
	void newDecisionLevel(){};
	bool propagateTheory(vec<Lit> & conflict){return true;};
	bool solve(vec<Lit> & conflict){return true;};

	void buildReason(Lit p, vec<Lit> & reason){};


	Lit newEdge(int from,int to)
    {
    		assert(isNode(from));assert(isNode(to));
    		Lit l = mkLit( S->newVar(),false);
    		Edge e{l,from,to};
    		edges[from].push(e);
    		return l;
    	}
    vec<Lit> c;
	void reachesAny(int from, vec<Lit> & reaches,int within_steps=-1){
		if(within_steps<0){
			within_steps=nodes;
		}
		//could put these in a separate solver to better test this...

        //bellman-ford:
        //Bellman-ford: For each vertex
		reaches.clear();
		for(int i = 0;i<nNodes();i++){
			reaches.push(False);
		}

		reaches[from]=True;

        for (int i = 0;i<within_steps;i++){
            //For each edge:
        	for(int j = 0;j<edges.size();j++){
        		for(int k = 0;k<edges.size();k++){
					Edge e = edges[j][k];
					if(reaches[e.to]==True){
						//do nothing
					}else if (reaches[e.from]==False){
						//do nothing
					}else{
						Lit r = mkLit( S->newVar(), false);
						c.clear();
						c.push(~r);c.push(reaches[e.to]);c.push(e.l); //r -> (e.l or reaches[e.to])
						S->addClause(c);
						c.clear();
						c.push(~r);c.push(reaches[e.to]);c.push(reaches[e.from]); //r -> (reaches[e.from]) or reaches[e.to])
						S->addClause(c);
						c.clear();
						c.push(r);c.push(~reaches[e.to]); //~r -> ~reaches[e.to]
						S->addClause(c);
						c.clear();
						c.push(r);c.push(~reaches[e.from]);c.push(~e.l); //~r -> (~reaches[e.from] or ~e.l)
						S->addClause(c);
						reaches[e.to]=r   ;//reaches[e.to] == (var & reaches[e.from])| reaches[e.to];
					}
        		}
        	}
        }
    }

};

};

#endif /* TESTGRAPH_H_ */