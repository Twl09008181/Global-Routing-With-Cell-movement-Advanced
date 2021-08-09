#ifndef ROUTING_HPP
#define ROUTING_HPP

#include "data_structure.hpp"
#include "graph.hpp"
#include <vector>
#include <utility>
#include <list>
#include <map>
#include <set>
#include <queue>
#include <string>
#include <iostream>
#include <algorithm>
#include <float.h>

bool add_segment_3D(Ggrid&P1,Ggrid&P2,Graph&graph,int NetID);

struct pos{
    int row,col,lay;
};
inline std::ostream& operator<<(std::ostream&os,pos&p)
{
    os<<p.row<<" "<<p.col<<" "<<p.lay;
    return os;
}
struct tree;
struct node{
    pos p;
    tree * routing_tree = nullptr;
    node* In[4] = {nullptr};//In[0] via from down,In[1]:via from up , In[2] negative dir of layer ,In[3] positive dir of layer
    bool IsLeaf = true;
    bool mark = false;//for print segment
    float cost = FLT_MAX;
    bool IsIntree = false;
};
struct tree{
    std::list<node*>leaf;
    std::list<node*>all;
};

using TwoPinNet = std::pair<node*,node*>;
using TwoPinNets = std::list<TwoPinNet>;





void Enroll(Ggrid&grid,Net*net);
void removedemand(Ggrid&grid,Net&net);
void addingdemand(Ggrid&grid,Net&net);
void RipUPinit(Graph*graph,Net&net);
void RoutingInit(Graph*graph,Net&net,TwoPinNets&pinset);
void SegmentFun(Graph*graph,Net&net,node*v,node*u,void(*f)(Ggrid&,Net&));

void Dfs_Segment(Graph*graph,Net&net,node*v,void(*f)(Ggrid&,Net&));

void RipUp(Graph*graph,Net&net,tree*t);

void printgrid(Ggrid&g,Net&net);


void printTree(Graph*graph,Net&net);

#endif
