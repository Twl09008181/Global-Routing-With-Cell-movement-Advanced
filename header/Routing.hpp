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

bool add_segment_3D(Ggrid&P1,Ggrid&P2,Graph&graph,int NetID);//未來將移除


//----------------------------------------------pos related----------------------------------------------------
struct pos{
    int row,col,lay;
};
inline std::ostream& operator<<(std::ostream&os,pos&p)
{
    os<<p.row<<" "<<p.col<<" "<<p.lay;
    return os;
}
inline std::string pos2str(const pos &p)
{
    std::string str = std::to_string(p.row)+" "+std::to_string(p.col)+" "+std::to_string(p.lay);
    return str;
}
//----------------------------------------------pos related----------------------------------------------------


//-----tree & node
struct tree;
struct node{
    pos p;
    tree * routing_tree = nullptr;
    node* In[4] = {nullptr};//In[0] via from down,In[1]:via from up , In[2] negative dir of layer ,In[3] positive dir of layer
    node* Out[4] = {nullptr};

    void unregister(node*host);
    bool IsLeaf();
    bool IsSingle();
    void connect(node *host);

    bool mark = false;//保留中
    float cost = FLT_MAX;
    bool IsIntree = false;
};
struct tree{
    std::list<node*>leaf;
    std::list<node*>all;
};

using TwoPinNet = std::pair<node*,node*>;
using TwoPinNets = std::list<TwoPinNet>;


//-----------All interface





//call back function
void Enroll(Ggrid&grid,Net*net);
void removedemand(Ggrid&grid,Net*net);
void addingdemand(Ggrid&grid,Net*net);
void Unregister(Ggrid&g,Net*net);


////Edge dfs tool (not vertex dfs)
using In = node**;
using InStorage = std::vector<In>;
InStorage getStorage(tree*nettree);
void RecoverIn(tree*nettree,InStorage&storage);

//Segment fun
void SegmentFun(Graph*graph,Net*net,node*v,node*u,void(*f)(Ggrid&,Net*));
void Dfs_Segment(Graph*graph,Net*net,node*v,void(*f)(Ggrid&,Net*));

//Demand interface
void DemandInterface(Graph*graph,Net*net,const std::string &operation);


//output interface
void printTreedfs(node*v,std::vector<std::string>*segment=nullptr);
void printTree(Graph*graph,Net*net,std::vector<std::string>*segment=nullptr);

//two pin net (untested)
void RoutingInit(Graph*graph,Net*net,TwoPinNets&pinset);

#endif
