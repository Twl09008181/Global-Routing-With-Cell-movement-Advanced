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

using TwoPinNet = std::pair<node*,node*>;
using TwoPinNets = std::list<TwoPinNet>;


//-----------All interface


//Ggrid callback function
bool EnrollNocheck(Ggrid&g,Net*net);
bool removedemand(Ggrid&grid,Net*net);
bool addingdemand(Ggrid&grid,Net*net);
bool Unregister(Ggrid&g,Net*net);

//Tree2Tree callback
bool Enroll(Ggrid&grid,Net*net);
bool target(Ggrid&g,Net*net);
bool Untarget(Ggrid&g,Net*net);
bool sourceInit(Ggrid&g,Net*net);

//Ggrid Segment fun
void Sgmt_Grid(Graph*graph,Net*net,node*v,node*u,bool(*f)(Ggrid&,Net*));
void Backtrack_Sgmt_Grid(Graph*graph,Net*net,node*v,bool(*f)(Ggrid&,Net*));



//node Segment fun
void ExpandTree(tree*t);


struct Par
{
    Par(bool (*f)(Ggrid&,Net*),Net::state s1 =Net::state::dontcare,Net::state s2 = Net::state::doneAdd )
        :callback{f}
    {
        Stating[0] = s1;
        Stating[1] = s2;
    }
    std::string warning;//warning
    Net::state Stating[2];//[0]:invalid state , [1] :valid state , [2]: change state
    bool (*callback)(Ggrid&,Net*);
};

//Tree interface
void RipUpAll(Graph*graph);
void AddingAll(Graph*graph);



void TreeInterface(Graph*graph,Net*net,Par par,tree* nettree=nullptr);


//RipUP and Adding
inline void RipUpNet(Graph*graph,Net*net,tree*nettree=nullptr){
    Par init(EnrollNocheck,Net::state::doneAdd,Net::state::RipUpinit);
    init.warning = "Warning : Net.routing state must be  Net::state::doneAdd!!\n";
    TreeInterface(graph,net,init,nettree);
    Par ripup(removedemand,Net::state::RipUpinit,Net::state::CanAdd);
    TreeInterface(graph,net,ripup,nettree);
}
inline void AddingNet(Graph*graph,Net*net,tree*nettree=nullptr){
    Par init(addingdemand,Net::state::CanAdd,Net::state::doneAdd);
    init.warning = "Warning : Net.routing state must be  Net::state::CanAdd!!\n";
    TreeInterface(graph,net,init,nettree);
}

//Tree2Tree
inline void SourceTree(Graph*graph,Net*net,tree*nettree){
    Par init(sourceInit,Net::state::Routing,Net::state::Routing);
    init.warning = "Warning : Net.routing state must be  Net::state::Routing!!\n";
    TreeInterface(graph,net,init,nettree);
}
inline void TargetTree(Graph*graph,Net*net,tree*nettree){
    Par init(target,Net::state::Routing,Net::state::Routing);
    init.warning = "Warning : Net.routing state must be  Net::state::Routing!!\n";
    TreeInterface(graph,net,init,nettree);
}
inline void UntargetTree(Graph*graph,Net*net,tree*nettree){
    Par init(Untarget,Net::state::Routing,Net::state::Routing);
    init.warning = "Warning : Net.routing state must be  Net::state::Routing!!\n";
    TreeInterface(graph,net,init,nettree);
}

//When Routing success
inline void UnRegisterTree(Graph*graph,Net*net,tree*nettree){
    Par init(Unregister,Net::state::Routing,Net::state::CanAdd);
    init.warning = "Warning : Net.routing state must be  Net::state::Routing!!\n";
    TreeInterface(graph,net,init,nettree);
}



//output interface
void backTrackPrint(node*v,std::string &NetName,std::vector<std::string>*segment=nullptr);
void printTree(tree*t,std::string &NetName,std::vector<std::string>*segment=nullptr);
void PrintAll(Graph*graph,std::vector<std::string>*segment=nullptr);
//two pin net (untested)
int TwoPinNetsInit(Graph*graph,Net*net,TwoPinNets&pinset);
int MovTwoPinNetsInit(Graph*graph,Net*net,TwoPinNets&pinset);//testing
#endif
