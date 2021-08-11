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
    bool IsSingle();
    void connect(node *host);

    bool mark = false;//保留中 
    float cost = FLT_MAX;
    bool IsIntree = false;
};
struct tree{
    tree()
    {
        for(int i = 0;i<4;i++)
            EndPoint.push_back(nullptr);
    }
    std::set<node*>leaf;//需要常常做update (有Out就要erase掉)
    std::list<node*>all;
    std::vector<Ggrid*>EndPoint;//Cellmoving要用到
    void addNode(node*n)
    {
        all.push_front(n);
        leaf.insert(n);
        n->routing_tree = this;
    }
    void updateEndPoint(Graph*graph)
    {
        for(auto n:all)
        {
            if(n->p.lay==-1)continue;
            auto &grid = (*graph)(n->p.row,n->p.col,n->p.lay);
            //Updating Endpoint
			if(!EndPoint.at(0)){EndPoint.at(0) = &grid;}
			if(!EndPoint.at(1)){EndPoint.at(1) = &grid;}
			if(!EndPoint.at(2)){EndPoint.at(2) = &grid;}
			if(!EndPoint.at(3)){EndPoint.at(3) = &grid;}
			int leftmost  = EndPoint.at(0)->col;
			int rightmost = EndPoint.at(1)->col;
			int bottom    = EndPoint.at(2)->row;
			int top       = EndPoint.at(3)->row;
			if(grid.col < leftmost)  EndPoint.at(0) = &grid;
			if(grid.col > rightmost) EndPoint.at(1) = &grid;
			if(grid.row < bottom)    EndPoint.at(2) = &grid;
			if(grid.row > top)       EndPoint.at(3) = &grid;
        }
    }
    ~tree()
    {
        //std::cout<<"tree destructor~\n";
        for(auto n:all)
        {
            //std::cout<<"delete "<<n->p<<"\n";
            delete n;
        }
    }
};

using TwoPinNet = std::pair<node*,node*>;
using TwoPinNets = std::list<TwoPinNet>;


//-----------All interface





//call back function
bool Enroll(Ggrid&grid,Net*net);
bool removedemand(Ggrid&grid,Net*net);
bool addingdemand(Ggrid&grid,Net*net);
bool Unregister(Ggrid&g,Net*net);


////Edge dfs tool (not vertex dfs)
using In = node**;
using InStorage = std::vector<In>;
InStorage getStorage(tree*nettree);
void RecoverIn(tree*nettree,InStorage&storage);

//Segment fun
void SegmentFun(Graph*graph,Net*net,node*v,node*u,bool(*f)(Ggrid&,Net*));
void Dfs_Segment(Graph*graph,Net*net,node*v,bool(*f)(Ggrid&,Net*));

//Tree interface
void TreeInterface(Graph*graph,Net*net,const std::string &operation,tree* nettree=nullptr);
void RipUpAll(Graph*graph);
void AddingAll(Graph*graph);
inline void RipUpNet(Graph*graph,Net*net,tree*nettree=nullptr){TreeInterface(graph,net,"RipUPinit",nettree);TreeInterface(graph,net,"RipUP",nettree);}
inline void AddingNet(Graph*graph,Net*net,tree*nettree=nullptr){TreeInterface(graph,net,"Adding",nettree);TreeInterface(graph,net,"doneAdd",nettree);}
inline void EnrollNet(Graph*graph,Net*net,tree*nettree=nullptr){TreeInterface(graph,net,"Enroll",nettree);}
inline void UnregisterNet(Graph*graph,Net*net,tree*nettree=nullptr){TreeInterface(graph,net,"Unregister",nettree);}



//output interface
void printTreedfs(node*v,std::vector<std::string>*segment=nullptr);
void printTree(Graph*graph,Net*net,std::vector<std::string>*segment=nullptr);
void PrintAll(Graph*graph);

//two pin net (untested)
int TwoPinNetsInit(Graph*graph,Net*net,TwoPinNets&pinset);

#endif
