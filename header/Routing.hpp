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

struct tree;
struct node{
    pos p;
    tree * routing_tree = nullptr;
    node* In[4] = {nullptr};
    bool IsLeaf = true;
    bool mark = false;//for print segment
    float cost = FLT_MAX;
    bool IsIntree = false;
};
struct tree{
    std::list<node*>leaf;
    std::list<node*>all;
};



#endif
