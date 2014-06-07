#ifndef LINK_CUT_COST
#define LINK_CUT_COST

#include <cstddef>
#include <cassert>
#include <climits>
#include <algorithm>
#include "mtl/Vec.h"
using namespace Minisat;
#ifdef NDEBUG
#define NDEBUG_LINKCUT
#endif
//This implementation of link/cut trees is based of a combination of D. Sleator's implementation (//http://codeforces.com/contest/117/submission/860934),
//and the (extremely well presented!) version described by Klein and Mozes, in the Appendix of Planarity.

//This link cut tree has been augmented to support operations for computing Maximum flows.
//Specifically, each edge in the abstract tree (ie, NOT the underlying splay tree) has an associated cost.
//For each node, we can compute in log time the cost of the edge connecting it to its parent, and also in log time compute
//the minimum cost of any edge on the path from the node to its parent.

//Note that is probably not the minimum cost that you might naturally expect the tree to compute: its is NOT the minimum cost of any node in the abstract tree beneath the node.
//This is especially a point of potential confusion, given that the way this minimum path cost is computed is by finding the minimum cost of any node in the concrete splay tree beneath the node in question (after exposing it).

 class LinkCutCost {
	static const int INF = INT_MAX/2;
	 int setCount;
  struct Node {

	int netcost=INF;//costs on the node are stored in 'delta' representation, following klein & mozes' description in the appendix in Planarity
	int netmin=0;
	bool hasRealParent=false;//used to skip splays in some cases.

#ifndef NDEBUG_LINKCUT
	int cost=INF;
	int min=INF;
	int dbg_min=INF;
	int dbg_parent=-1;
	int dbg_ID;
#endif
	int left=-1;
    int right=-1;
    int parent=-1;
    Node(int id){
#ifndef NDEBUG
    	dbg_ID = id;
#endif
    };


  };
  vec<Node> nodes;
  int grossmin(int  v){
	  dbg_min(v);
	  expose(v);
	  return nodes[v].netmin+nodes[v].netcost;
  }
  int grosscost(int  v){
	  //int cost = v.netcost + grossmin(v);
	  //assert(cost==v.cost);
#ifndef NDEBUG_LINKCUT
	  int cp = nodes[v].cost;
#endif
	  expose(v);
	  dbg_print_forest();
	  //int it = iter;
	  int cost = nodes[v].netcost;
#ifndef NDEBUG_LINKCUT
	  assert(cost==cp);
#endif
	  return cost;
	  //return v.cost;
  }
  inline void update(int v) {
#ifndef NDEBUG_LINKCUT
	  update(nodes[v]);
#endif
  }
  inline void update(Node & v) {
#ifndef NDEBUG_LINKCUT
     	int mincost = v.cost;

 		if (v.left>-1) {
 			mincost = std::min(mincost,nodes[v.left].min);
 		}
 		if (v.right>-1) {
 			mincost = std::min(mincost, nodes[v.right].min);
 		}
 		if(mincost==INF)
 			mincost=0;
 		v.min=mincost;

 		{
			v.dbg_min =v.cost;

			if (v.left>-1) {
				v.dbg_min = std::min(v.dbg_min, nodes[v.left].dbg_min);
			}
			if (v.right>-1) {
				v.dbg_min = std::min(v.dbg_min, nodes[v.right].dbg_min);
			}
			if(v.dbg_min==INF)
				v.dbg_min=0;
			assert(v.dbg_min==v.min);
 		}
 #endif
    }

  //Update the costs on every edge in the path of the tree from v to root v by delta.
  //has NO EFFECT if v is a root in the abstract tree (as roots have no edges on the path to themselves).
  void updatePathCost(int v, int delta){
	  expose(v);
	  nodes[v].netcost+=delta;
#ifndef NDEBUG_LINKCUT

	  while(v>-1){
		  nodes[v].cost+=delta;
		  if(nodes[v].cost<nodes[v].min){
			  nodes[v].min=nodes[v].cost;
			  nodes[v].dbg_min=nodes[v].cost;
		  }

		  v=nodes[v].dbg_parent;
	  }
#endif
	  dbg_min(v);

  }

  void dbg_min(int v){
#ifndef NDEBUG_LINKCUT
	  if(v==-1){
		  return;
	  }
	  //This debug code only checks correctly if v is exposed!
	  if(nodes[v].parent!=-1){
		  return;
	  }

	  int m =nodes[v].netmin;
	  int dbg_min = nodes[v].cost;
	  int p =v;

	  int c = nodes[v].netcost;

	  while(nodes[p].parent>-1){
		  if(nodes[p].dbg_ID==nodes[nodes[p].parent].left || nodes[p].dbg_ID==nodes[nodes[p].parent].right){
			  p=nodes[p].parent;

			  //m+=p.netmin;
			  c+=nodes[p].netcost;

		  }else{
			  break;
		  }
	  }

	  m+=c;//minw = delta_minw+w;

	  p =v;
	  dbg_min = std::min(dbg_min,nodes[p].cost);

	  while(nodes[p].dbg_parent>-1){

			  p=nodes[p].dbg_parent;

			  dbg_min = std::min(dbg_min,nodes[p].cost);

	  }

	  int minGrossCost =nodes[v].cost;

	  vec<int> Q;
	  Q.push(v);
	  while(Q.size()){
		  assert(Q.last() > -1);
		  Node & w = nodes[Q.last()];
		  Q.pop();
		  minGrossCost = std::min(minGrossCost,w.cost);
		  if(w.right>-1)
			  Q.push(w.right);

		  if(w.left>-1)
			  Q.push(w.left);
	  }
	 // if(minGrossCost==INF)
	//	  minGrossCost=0;
	  assert(m==dbg_min);
	  assert(m==minGrossCost);






/*	  while (v.right != NULL) {
	       v = v.right;
	     }
	  dbg_isGrossMin(v.min,v);*/
#endif
  }
  bool dbg_is_ancestor(int v, int p){
	  if (v==p){
		  return true;
	  }
	  while(nodes[v].parent>-1){
		  if(nodes[v].parent==p)
			  return true;
		  v=nodes[v].parent;
	  }
	  return false;
  }


  inline void dbg_print_forest(bool force = false){
#ifndef NDEBUG_LINKCUT
	  int iter = 0;
	  if(!force)
		  return;
	 /* if(++iter<= 1415550){
	 		 return;
	 	  }
	  if(iter== 1415555){
		  int a =1;
	  }*/

		printf("digraph{\n");
		for(int i = 0;i<nodes.size();i++){
			printf("n%d [label=\"%d: c%d, m%d\"]\n", i,i,nodes[i].netcost,nodes[i].min);
		}

		for(int i = 0;i<nodes.size();i++){

			Node & n = nodes[i];
			if(n.parent){
				const char * s = "black";
				if(n.dbg_ID==nodes[n.parent].left){
					s="blue";
					assert(n.dbg_ID!=nodes[n.parent].right);
				}

				if(n.dbg_ID==nodes[n.parent].right){
					s="red";
				}

				/*if(value(e.v)==l_True)
					s="blue";
				else if (value(e.v)==l_False)
					s="red";*/
				printf("n%d . n%d [label=\"%d: %d\",color=\"%s\"]\n", i,nodes[n.parent].dbg_ID, i, n.cost, s);
			}

		}

		printf("}\n");

#endif
  }

  void dbg_cost(int  v){
#ifndef NDEBUG_LINKCUT
	  if(v==-1)
		  return;
	  int c = nodes[v].netcost;
	  int p =v;
	  while(nodes[p].parent>-1){
		  if(p==nodes[nodes[p].parent].left || p==nodes[nodes[p].parent].right){
			  p=nodes[p].parent;
			  c+=nodes[p].netcost;
		  }else{
			  break;
		  }
	  }
	  assert(c==nodes[v].cost);
#endif
  }

  void dbg_all(){
#ifndef NDEBUG_LINKCUT
	  int i = 0;
	  for(Node & v:nodes){
		  int id = i++;
		  expose(id);
		  dbg_cost(id);
		  dbg_min(id);
	  }
#endif
  }

  void dbg_isGrossMin(int min,int v){
#ifndef NDEBUG_LINKCUT
	  static int iter = 0;
	  if(++iter==22349){
		  int a=1;
	  }
	 // dbg_print_forest();
	  int minGrossCost =nodes[v].cost;

	  vec<int> Q;
	 /* for(int i = 0;i<nodes.size();i++){
		  Node & y = nodes[i];
		  if(dbg_is_ancestor(y,v)){
			  minGrossCost = std::min(minGrossCost, grosscost(y));
			  assert(minGrossCost>=v.min);
		  }
	  }*/
	  Q.push(v);
	  while(Q.size()){
		  Node & w = nodes[Q.last()];
		  Q.pop();
		  minGrossCost = std::min(minGrossCost, w.cost);
		  if(w.right)
			  Q.push(w.right);

		  if(w.left)
			  Q.push(w.left);
	  }
	  if(minGrossCost==INF)
		  minGrossCost=0;
	  assert(minGrossCost == min);
#endif
  }



  // Whether x is a root of a splay tree
  bool isSplayRoot(int x) {
    return nodes[x].parent == -1 || (nodes[nodes[x].parent].left != x && nodes[nodes[x].parent].right != x);
  }

 /* void connect(Node &ch, Node &p, bool leftChild) {
    if (leftChild)
      p.left = ch;
    else
      p.right = ch;
    if (ch != NULL)
      ch.parent = p;
  }
*/

  void rotR (int  pID) {
	Node & p = nodes[pID];
	int qID = p.parent;
	Node & q = nodes[p.parent];
	int rID = q.parent;
	Node & r = nodes[q.parent];
#ifndef NDEBUG_LINKCUT
	int cp = p.cost;
	int cq = q.cost;
	//int cr = r? grosscost(r):0;
	int cb = p.right>-1? nodes[p.right].cost:0;
	dbg_cost(qID);dbg_cost(pID); dbg_cost(rID); dbg_cost(q.right);
#endif

	//delta weight/minweight update rules from Klein and Mozes's Planarity, chapter 17
	int deltap = p.netcost;
	int deltaq = q.netcost;
	int deltab = p.right>-1?nodes[p.right].netcost:0;

	if ((q.left=p.right) >-1){
		nodes[q.left].parent = qID;
	}
	p.right = qID;
	q.parent = pID;

	p.netcost+=deltaq;
	q.netcost= -deltap;
	if(q.left>-1){
		nodes[q.left].netcost+=deltap;
	}

	if ((p.parent=rID) >-1) {
	    if (r.left == qID){
	    	r.left = pID;
	    }else if (r.right == qID){
	    	r.right = pID;
	    }
	}


	update(q);
	childChange(qID);
	childChange(pID);

	dbg_cost(qID);dbg_cost(pID); dbg_cost(rID); dbg_cost(q.right);
	//dbg_min(q);dbg_min(p);dbg_min(r); dbg_min(q.right);
 }

  void rotL (int pID) {
	Node & p = nodes[pID];
	int qID = p.parent;
	Node & q = nodes[p.parent];
	int rID = q.parent;
	Node & r = nodes[q.parent];
#ifndef NDEBUG_LINKCUT
	int cp = p.cost;
	int cq = q.cost;
	//int cr = r? grosscost(r):0;
	int cb = p.left>-1? nodes[p.left].cost:0;
	dbg_cost(qID);dbg_cost(pID); dbg_cost(rID);
#endif

	int deltap = p.netcost;
	int deltaq = q.netcost;
	int deltab = p.left>-1?nodes[p.left].netcost:0;

	if ((q.right=p.left) >-1){
		nodes[q.right].parent = qID;
	}
	p.left = qID;
	q.parent = pID;

	if ((p.parent=rID) >-1) {
	    if (r.left == qID){
	    	r.left = pID;
	    }else if (r.right == qID){
	    	r.right = pID;
	    }
	}

	p.netcost+=deltaq;
	q.netcost= -deltap;
	if(q.right>-1){
		nodes[q.right].netcost+=deltap;
	}


	update(q);
	childChange(qID);
	childChange(pID);

	dbg_cost(qID);dbg_cost(pID); dbg_cost(rID); dbg_cost(q.right);
	//dbg_min(q);dbg_min(p);dbg_min(r); dbg_min(q.right);
 }

  void splay(int pID) {
	  Node & p = nodes[pID];
#ifndef NDEBUG_LINKCUT
	  dbg_print_forest();
	  int cp = p.cost;
#endif
 	while (!isSplayRoot(pID)) {
 	    int qID = p.parent;
 	    if (isSplayRoot(qID)) {
 		if (nodes[qID].left == pID) rotR(pID); else rotL(pID);
 	    } else {
 	    int rID = nodes[qID].parent;
 		if (nodes[rID].left == qID) {
 		    if (nodes[qID].left == pID) {rotR(qID); rotR(pID);}
 		    else {rotL(pID); rotR(pID);}
 		} else {
 		    if (nodes[qID].right == pID) {rotL(qID); rotL(pID);}
 		    else {rotR(pID); rotL(pID);}
 		}
 	    }
 	}

 	update(p);

     }

  inline void childChange(int xID){
	  Node & x = nodes[xID];
	  x.netmin = std::min(0,x.left>-1? (nodes[x.left].netmin + nodes[x.left].netcost) :0);
	  x.netmin = std::min(x.netmin,x.right>-1? (nodes[x.right].netmin + nodes[x.right].netcost) :0);
  }

  // Makes node x the root of the virtual tree, and also x is the leftmost
  // node in its splay tree
   int expose(int x) {
    int last = -1;
    for (int y = x; y > -1; y = nodes[y].parent) {
    	dbg_cost(y);
      splay(y);
      if(nodes[y].left>-1){
    	  nodes[nodes[y].left].netcost+=nodes[y].netcost;
      }
      nodes[y].left = last;
      if(last>-1){
    	  nodes[last].netcost-=nodes[y].netcost;
      }
      childChange(y);
      update(y);
      last = y;

    }
    dbg_cost(x);
    splay(x);

    dbg_cost(x);
    dbg_min(x);

    return last;
  }


  inline int _findRoot(int x) {
   if(!nodes[x].hasRealParent)
	   return x;
    expose(x);
    while (nodes[x].right >-1) {
      x = nodes[x].right;
    }
    //splay(x);
    return x;
  }

   bool dbgSetCount(){
#ifndef NDEBUG
	   int count = 0;
	   for(int i = 0;i<nodes.size();i++){
		   int n =i;
		   int r = _findRoot(n);
		   if(r==n){
			   count++;
		   }
	   }
	   return count==setCount;
#endif
	   return true;
   }

  // prerequisite: x and y are in distinct trees, AND x is the root of its tree
    void _link(int x,int y, int cost=0) {
    //assert (_findRoot(x) != _findRoot(y));
#ifndef NDEBUG_LINKCUT
	int sY = _findRoot(y);
    int sX = _findRoot(x);
    assert(sY!=sX);//else these are already linked.
    assert(x==sX);

#endif

    setCount--;
    expose(x);

    assert(nodes[x].parent==-1);
#ifndef NDEBUG_LINKCUT
    //assert(nodes[x].cost==INF);
#endif
    nodes[x].parent = y;
    nodes[x].netcost= cost;
    nodes[x].hasRealParent=true;
#ifndef NDEBUG_LINKCUT

	if(nodes[x].left==-1 && nodes[x].right==-1){
		assert(nodes[x].netmin==0);
		nodes[x].min=cost;
	}else{
		nodes[x].min=std::min(nodes[x].min,cost);
	}
#endif
	nodes[x].netmin = std::min(nodes[x].netmin,cost);

#ifndef NDEBUG_LINKCUT
	nodes[x].cost=cost;
	nodes[x].dbg_parent = y;
#endif
    assert(grosscost(x)==cost);
    assert(dbgSetCount());

    dbg_min(y);
    dbg_all();
  }

    bool _connected(int x, int y) {
    if (x == y)
      return true;
    expose(x);
    expose(y);
    return nodes[x].parent != -1;
  }
    void _cut(int xID){
    	Node & x = nodes[xID];
    	expose(xID);
    	assert(x.right>-1);//else, x is a root node
    	nodes[x.right].netcost+=x.netcost;
    	nodes[x.right].parent=-1;
    	update(x.right);
    	//childChange(x.right);
    	x.right=-1;
    	x.netcost=INF;
    	x.hasRealParent=false;
#ifndef NDEBUG_LINKCUT
    	x.cost=INF;
    	x.dbg_parent = -1;
#endif
    	childChange(xID);
    	update(x);
    	setCount++;
    	dbg_all();
    }

    //u is the node to search from; a is a weight greater to or equal to the delta_minw(s)
	//returns the rightmost (or leftmost, if leftdir is true) solid descendent v of u such that w(v) <= a+w(u)
	int solidFind(int u, int alpha, bool leftdir){
		assert(u>-1);
		assert(alpha>=nodes[u].netmin);
		int u1 = nodes[u].left;
		int u2 = nodes[u].right;
		if(!leftdir){
			std::swap(u1,u2);
		}
		if(u1>-1 && (alpha-nodes[u1].netcost) >= nodes[u1].netmin){
			return solidFind(u1,alpha-nodes[u1].netcost,leftdir);
		}
		if (alpha>=0){
			splay(u);
			return u;
		}
		return solidFind(u2,alpha-nodes[u2].netcost,leftdir);
	}



public:
    LinkCutCost():setCount(0){

    }

   int addNode(){
	   //return new Node();
	   setCount++;
	   nodes.push(nodes.size());//new Node(nodes.size()));
	   return nodes.size()-1;
   }
   int nNodes(){
	   return nodes.size();
   }

  inline int findRoot(int x) {
    int r= _findRoot(x);
    dbg_min(x);
    return r;
  }

  // prerequisite: x and y are in distinct trees. y becomes the parent of x.
    void link(int x, int y, int cost = 0) {
    	if(x==y)
    		return;
    	_link(x,y,cost);
  }

    bool connected(int x, int y) {
    if (x == y)
      return true;

     expose(x);
	 expose(y);
     return nodes[x].parent != -1;
  }

    //Returns the cost of the edge connecting x to the tree; x must not be a root.
    int getCost(int x){
    	return grosscost(x);
    }

    //Returns the lowest cost in the tree rooted at x.
    //x must be a root!
    int minCost(int x){
    	return grossmin(x);
    }

    //Find the minimum weight ancestor of s that is either closest to root(s) (if leftdir is false), or closest to s.
	int ancecstorFindMin(int u,bool leftdir=false){
		expose(u);
		Node & p = nodes[u];
		int alpha = p.netcost+ p.netmin;
		bool u_is_candidate = p.netcost <=alpha;
		bool candidate_on_right = p.right>-1 && (nodes[p.right].netmin+nodes[p.right].netcost+p.netcost <= alpha);
		if (u_is_candidate && (leftdir || !candidate_on_right)){
			return u;
		}
		if(!candidate_on_right)
			return -1;
		return solidFind(p.right,alpha-nodes[p.right].netcost - p.netcost,leftdir);
	}
	//Find the ancestor of s with the lowest weight.
	int ancecstorFindWeight(int  u,int alpha, bool leftdir){
		expose(u);
		Node & p = nodes[u];

		bool u_is_candidate = nodes[u].netcost <=alpha;
		bool candidate_on_right = p.right>-1 && (nodes[p.right].netmin+ nodes[p.right].netcost+p.netcost <= alpha);
		if (u_is_candidate && (leftdir || !candidate_on_right)){
			return u;
		}
		if(!candidate_on_right)
			return -1;
		return solidFind(p.right,alpha-nodes[p.right].netcost - p.netcost,leftdir);
	}


    //updates the cost of each element in x's path to root.
    void updateCostOfPathToRoot(int x, int delta){
    	updatePathCost(x,delta);
    }

    void cut(int x){
      	_cut(x);
    	dbg_min(x);
    }
    //True if u is a root in the forest
    bool isRoot(int u){
    	return !nodes[u].hasRealParent;
    }

    int numRoots(){
    	assert(dbgSetCount());
    	return setCount;
    }

    void reset(){
    	for(int i = 0;i<nodes.size();i++){
    		(nodes[i]).~Node();
    		new (&nodes[i]) Node(i);
    	}
    	setCount= nodes.size();
    }

};
#endif
