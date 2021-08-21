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
    float cost = FLT_MAX;
    bool IsIntree = false;//backtrack need

};
struct tree{
    tree()
    {
        for(int i = 0;i<6;i++)
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
            for(int i = 0;i<6;i++)
            {
                if(!EndPoint.at(i)){EndPoint.at(i) = &grid;}
            }
			int minCol  = EndPoint.at(0)->col;
			int maxCol = EndPoint.at(1)->col;
			int minRow = EndPoint.at(2)->row;
			int maxRow = EndPoint.at(3)->row;
            int minLay = EndPoint.at(4)->lay;
            int maxLay = EndPoint.at(5)->lay;
			if(grid.col < minCol)  EndPoint.at(0) = &grid;
			if(grid.col > maxCol ) EndPoint.at(1) = &grid;
			if(grid.row < minRow)  EndPoint.at(2) = &grid;
			if(grid.row > maxRow)  EndPoint.at(3) = &grid;
            if(grid.lay < minLay)  EndPoint.at(4) = &grid;
			if(grid.lay > maxLay)  EndPoint.at(5) = &grid;
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
    std::unordered_map<Ggrid*,bool>grids;
    int NetId;
    float passScore = 0;
};

using TwoPinNet = std::pair<node*,node*>;
using TwoPinNets = std::list<TwoPinNet>;


//-----------All interface


//Ggrid callback function
bool passing(Ggrid*grid,NetGrids*net);//only for segment form (initial routing)
//Ggrid Segment fun
void Sgmt_Grid(Graph*graph,NetGrids*net,node*v,node*u,bool(*f)(Ggrid* ,NetGrids*));
void Backtrack_Sgmt_Grid(Graph*graph,NetGrids*net,node*v,bool(*f)(Ggrid* ,NetGrids*));
void TreeInterface(Graph*graph,NetGrids*net,bool(*f)(Ggrid* ,NetGrids*),tree* nettree=nullptr);
inline void AddSegment(Graph*graph,NetGrids*net,tree*nettree=nullptr){
    TreeInterface(graph,net,passing,nettree);
}



//Demand

float RipUpNet(Graph*graph,NetGrids*net);
float AddingNet(Graph*graph,NetGrids*net);

void RipUpAll(Graph*graph);
void AddingAll(Graph*graph);


//output interface
void backTrackPrint(Graph*graph,Net*net,node*v,std::vector<std::string>*segment=nullptr);
void printTree(Graph*graph,Net*net,tree*t,std::vector<std::string>*segment=nullptr);
void PrintAll(Graph*graph,std::vector<std::string>*segment=nullptr);
//two pin net (untested)
int TwoPinNetsInit(Graph*graph,NetGrids*net,TwoPinNets&pinset);



//Routing Interface
struct ReroutInfo{
    tree* nettree;
    NetGrids* netgrids;
};

std::pair<ReroutInfo,bool> Reroute(Graph*graph,int NetId,TwoPinNets&twopins);



struct BoundingBox
{
    BoundingBox() = default;
    BoundingBox(Graph*graph,Net*net)
        :RowBound{graph->RowBound()},ColBound{graph->ColBound()}
    {
        LayBound.first = net->minLayer;
        LayBound.second = graph->LayerNum();
        minRow = RowBound.first;
        maxRow = RowBound.second;
        minCol = ColBound.first;
        maxCol = ColBound.second;
        minLay = LayBound.first;
        maxLay = LayBound.second;
    }
    BoundingBox(Graph*graph,Net*net,tree*t1,tree*t2)
    {

       
        RowBound = graph->RowBound();
        ColBound = graph->ColBound();
        LayBound.first = net->minLayer;
        LayBound.second = graph->LayerNum();
        
        minCol = min(t1->EndPoint.at(0)->col,t2->EndPoint.at(0)->col);
        minCol = max(minCol-flexCol,ColBound.first);

        maxCol = max(t1->EndPoint.at(1)->col,t2->EndPoint.at(1)->col);
        maxCol = min(maxCol+flexCol,ColBound.second);



        minRow = min(t1->EndPoint.at(2)->row,t2->EndPoint.at(2)->row);
        minRow = max(minRow-flexRow,RowBound.first);

        maxRow = max(t1->EndPoint.at(3)->row,t2->EndPoint.at(3)->row);
        maxRow = min(maxRow+flexRow,RowBound.second);

        minLay = min(t1->EndPoint.at(4)->lay,t2->EndPoint.at(4)->lay);
        minLay = max(minLay-flexLay,LayBound.first);

        maxLay = max(t1->EndPoint.at(5)->lay,t2->EndPoint.at(5)->lay);
        maxLay = min(maxLay+flexLay,LayBound.second);
    }
    void loosen()
    {
        flexRow+=10;
        flexCol+=10;
        flexLay+=10;
    }
    std::pair<int,int>RowBound;
    std::pair<int,int>ColBound;
    std::pair<int,int>LayBound;
    int minCol,maxCol,minRow,maxRow,minLay,maxLay;
    int flexRow = 5,flexCol = 5,flexLay = 5;
};




#endif
