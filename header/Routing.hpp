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
    bool operator==(pos&other)
    {
        return row==other.row&&col==other.col&&lay==other.lay;
    }
    bool operator!=(pos&other)
    {
        return !(*this==other);
    }
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
    node(const pos&position)
        : p{position},routing_tree{nullptr},parent{nullptr}{}
    node()
        :routing_tree{nullptr},parent{nullptr}{}
    
    bool IsSingle();//not parent , and not child (single grid net)
    void connect(node *host);
    pos p;
    tree * routing_tree;
    node* parent;
    std::set<node*>child;
    bool mark = false;//保留中 
    float cost = FLT_MAX;
    bool IsIntree = false;//backtrack need
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
        for(auto n:all){delete n;}
    }
};

struct NetGrids
{
    NetGrids(int netId)
    :NetId{netId}{}

    bool AlreadyPass(Ggrid*grid)
    {
        return grids.find(grid)==grids.end() ? false:true;
    }
    void PassGrid(Ggrid*grid)
    {
        grids[grid] = false; //set to false
    }
    bool Add(Ggrid*grid)
    {
        if(!grids.at(grid))
        {
            if(grid->get_remaining())
            {
                grids.at(grid) = true;
                grid->add_demand();
            }
            else{
                return false;//canAdd
            }
        }
        return true;//AlreadyAdd
    }
    std::unordered_map<Ggrid*,bool>grids;
    int NetId;
};

using TwoPinNet = std::pair<node*,node*>;
using TwoPinNets = std::list<TwoPinNet>;


//-----------All interface


//Ggrid callback function
bool passing(Ggrid*grid,NetGrids*net);//only for segment form (initial routing)
bool target(Ggrid* g,NetGrids*net);
bool Untarget(Ggrid* g,NetGrids*net);


//Ggrid Segment fun
void Sgmt_Grid(Graph*graph,NetGrids*net,node*v,node*u,bool(*f)(Ggrid* ,NetGrids*));
void Backtrack_Sgmt_Grid(Graph*graph,NetGrids*net,node*v,bool(*f)(Ggrid* ,NetGrids*));




//Tree interface

void RipUpNet(Graph*graph,NetGrids*net);
void AddingNet(Graph*graph,NetGrids*net);

void RipUpAll(Graph*graph);
void AddingAll(Graph*graph);



void TreeInterface(Graph*graph,NetGrids*net,bool(*f)(Ggrid* ,NetGrids*),tree* nettree=nullptr);


// //RipUP and Adding
// inline void RipUpNet(Graph*graph,NetGrids*net,tree*nettree=nullptr){
//     TreeInterface(graph,net,removedemand,nettree);
// }

inline void AddSegment(Graph*graph,NetGrids*net,tree*nettree=nullptr){
    TreeInterface(graph,net,passing,nettree);
}

inline void TargetTree(Graph*graph,NetGrids*net,tree*nettree){
    TreeInterface(graph,net,target,nettree);
}
inline void UntargetTree(Graph*graph,NetGrids*net,tree*nettree){
    TreeInterface(graph,net,Untarget,nettree);
}

// void backTrackPrint(node*v,Net*net,std::vector<std::string>*segment)
//output interface
void backTrackPrint(Graph*graph,Net*net,node*v,std::vector<std::string>*segment=nullptr);
void printTree(Graph*graph,Net*net,tree*t,std::vector<std::string>*segment=nullptr);
void PrintAll(Graph*graph,std::vector<std::string>*segment=nullptr);
//two pin net (untested)
int TwoPinNetsInit(Graph*graph,NetGrids*net,TwoPinNets&pinset);




//Routing Interface
std::pair<tree*,bool> Reroute(Graph*graph,NetGrids*net,TwoPinNets&twopins);








#endif
