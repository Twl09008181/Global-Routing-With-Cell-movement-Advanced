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

struct table{

    void init(Graph*graph)
    {
        mr = graph->RowBound().first;
        Mr = graph->RowBound().second;
        mc = graph->ColBound().first;
        Mc = graph->ColBound().second;
        strTable = new std::vector<std::string>(graph->LayerNum()*(Mr-mr+1)*(Mc-mc+1));
        for(int l =1;l<=graph->LayerNum();l++)
            for(int r = mr;r<=Mr;r++)
                for(int c = mc;c<=Mc;c++)
                    strTable->at( (l-1)*(Mr-mr+1)*(Mc-mc+1) + (r-mr)*(Mc-mc+1) + c-mc ) = std::to_string(r) + " "+std::to_string(c)+" "+std::to_string(l);
    }

    std::vector<std::string> *strTable = nullptr;
    int mr,Mr,mc,Mc;
    std::string& get(int r,int c,int l)
    {
        return strTable->at( (l-1)*(Mr-mr+1)*(Mc-mc+1) + (r-mr)*(Mc-mc+1) + c-mc );
    }
};
extern table strtable;
inline std::string& pos2str(const pos &p)
{
    // std::string str = std::to_string(p.row)+" "+std::to_string(p.col)+" "+std::to_string(p.lay);
    return strtable.get(p.row,p.col,p.lay);
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
			int _minCol  = EndPoint.at(0)->col;
			int _maxCol = EndPoint.at(1)->col;
			int _minRow = EndPoint.at(2)->row;
			int _maxRow = EndPoint.at(3)->row;
            int _minLay = EndPoint.at(4)->lay;
            int _maxLay = EndPoint.at(5)->lay;
			if(grid.col < _minCol)  EndPoint.at(0) = &grid;
			if(grid.col > _maxCol ) EndPoint.at(1) = &grid;
			if(grid.row < _minRow)  EndPoint.at(2) = &grid;
			if(grid.row > _maxRow)  EndPoint.at(3) = &grid;
            if(grid.lay < _minLay)  EndPoint.at(4) = &grid;
			if(grid.lay > _maxLay)  EndPoint.at(5) = &grid;
        }
    }
    ~tree()
    {
        for(auto n:all){delete n;}
    }
};

struct NetGrids
{
    NetGrids(int netId,bool overflowAllow=false)
    :NetId{netId},overflow_mode{overflowAllow}
    {}

    bool AlreadyPass(Ggrid*grid)
    {
        return grids.find(grid)==grids.end() ? false:true;
    }
    void PassGrid(Ggrid*grid)
    {
        grids[grid] = false; //set to false
    }
    void PassGrid(Graph*graph,node*n)
    {
        auto &grid = (*graph)(n->p.row,n->p.col,n->p.lay);
        PassGrid(&grid);
    }
    int wl()const{return grids.size();}
    //overflow flag
    void ResetFlag()
    {
        overflow_mode = false;
        is_overflow = false;
        recover_mode = false;
    }
    bool isOverflow()const{return is_overflow;}
    void overflow(){is_overflow = true;}
    std::unordered_map<Ggrid*,bool>grids;
    int NetId;
    float passScore = 0;

    //fixed flag , avoid duplicate routing without Accept/Reject
    void set_fixed(bool fixed){is_fixed = fixed;}
    bool isFixed(){return is_fixed;}
    bool is_fixed = false;
    

    bool overflow_mode = false;
    bool is_overflow = false;
    bool recover_mode = false;
    
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
TwoPinNets twoPinsGen(Net&net,int defaultLayer=0);//refactor , set defaultLayer = 0


//Routing Interface
struct ReroutInfo{
    tree* nettree;
    NetGrids* netgrids;
};

std::pair<ReroutInfo,bool> Reroute(Graph*graph,int NetId,TwoPinNets&twopins,bool overflowMode=false);


#include <climits>
struct BoundingBox
{
    BoundingBox() = default;
    BoundingBox(Graph*graph,Net*net)//all
        :RowBound{graph->RowBound()},ColBound{graph->ColBound()}
    {
        LayBound.first = net->minLayer;
        LayBound.second = graph->LayerNum();
        _minRow = RowBound.first;
        _maxRow = RowBound.second;
        _minCol = ColBound.first;
        _maxCol = ColBound.second;
        _minLay = LayBound.first;
        _maxLay = LayBound.second;
    }
    BoundingBox(Graph*graph,Net*net,node*n1,node*n2)
    {
        RowBound = graph->RowBound();
        ColBound = graph->ColBound();
        LayBound.first = net->minLayer;
        LayBound.second = graph->LayerNum();

        _minCol = min(n1->p.col,n2->p.col);
        _maxCol = max(n1->p.col,n2->p.col);
        _minRow = min(n1->p.row,n2->p.row);
        _maxRow = max(n1->p.row,n2->p.row);
     
        _minLay = LayBound.first;
        _maxLay = LayBound.second;
    }
    BoundingBox(Graph*graph,Net*net,tree*t1,tree*t2)
    {
        RowBound = graph->RowBound();
        ColBound = graph->ColBound();
        LayBound.first = net->minLayer;
        LayBound.second = graph->LayerNum();
        //改成兩者之間最近的兩個點
        int dist = INT_MAX;
        node*_n1 = nullptr;
        node*_n2 = nullptr;
        for(auto n1:t1->all)
        {
            for(auto n2:t2->all)
            {
                int d_r = (n1->p.row-n2->p.row);
                int d_c = (n1->p.col-n2->p.col);
                if(d_r*d_r+d_c*d_c < dist)
                {
                    dist = d_r*d_r+d_c*d_c;
                    _n1 = n1;
                    _n2 = n2;
                }
            }
        }
        _minCol = min(_n1->p.col,_n2->p.col);
        _maxCol = max(_n1->p.col,_n2->p.col);
        _minRow = min(_n1->p.row,_n2->p.row);
        _maxRow = max(_n1->p.row,_n2->p.row);


        _minLay = LayBound.first;
        _maxLay = LayBound.second;
    }
    BoundingBox(Graph*graph,Net*net,tree*t1,node*n2)
    {
        RowBound = graph->RowBound();
        ColBound = graph->ColBound();
        LayBound.first = net->minLayer;
        LayBound.second = graph->LayerNum();
        
        int dist = INT_MAX;
        node*_n1 = nullptr;
        for(auto n1:t1->all)
        {
            int d_r = (n1->p.row-n2->p.row);
            int d_c = (n1->p.col-n2->p.col);
            if(d_r*d_r+d_c*d_c < dist)
            {
                dist = d_r*d_r+d_c*d_c;
                _n1 = n1;
            }
        }
        _minCol = min(_n1->p.col,n2->p.col);
        _maxCol = max(_n1->p.col,n2->p.col);
        _minRow = min(_n1->p.row,n2->p.row);
        _maxRow = max(_n1->p.row,n2->p.row);

        _minLay = LayBound.first;
        _maxLay = LayBound.second;
    }
    void getBound(int &maxRow,int &minRow,int &maxCol,int &minCol,int &maxLay,int &minLay)
    {
        minCol = max(_minCol-flexCol,ColBound.first);
        maxCol = min(_maxCol+flexCol,ColBound.second);
        minRow = max(_minRow-flexRow,RowBound.first);
        maxRow = min(_maxRow+flexRow,RowBound.second);
        minLay = max(_minLay-flexLay,LayBound.first);
        maxLay = min(_maxLay+flexLay,LayBound.second);
    }
    bool loosen()
    {
        bool update = false;
        if(!init){init = true;return true;}
        if(_minCol-flexCol>ColBound.first){flexCol++;update=true;}
        if(_maxCol+flexCol<ColBound.second){flexCol++;update=true;}
        if(_minRow-flexRow>RowBound.first){flexRow++;update=true;}
        if(_maxRow+flexCol<RowBound.second){flexRow++;update=true;}
        if(_minLay-flexLay>LayBound.first){flexLay++;update=true;}
        if(_maxLay+flexLay<LayBound.second){flexLay++;update=true;}
        
     
        return update;
    }
    void initFlex(int flex = 3)
    {
        flexRow = flex;
        flexCol = flex;
        flexLay = flex;

        //tune
        if(_minCol-flexCol < ColBound.first){flexCol = _minCol-ColBound.first;}
        if(_maxCol+flexCol > ColBound.second){flexCol = ColBound.second-_maxCol;}
        if(_minRow-flexRow < RowBound.first){flexRow = _minRow-RowBound.first;}
        if(_maxRow+flexCol > RowBound.second){flexRow = RowBound.second - _maxRow;}
        if(_minLay-flexLay < LayBound.first){flexLay = _minLay-LayBound.first;}
        if(_maxLay+flexLay > LayBound.second){flexLay = LayBound.second - _maxLay;}
    }
    std::pair<int,int>RowBound;
    std::pair<int,int>ColBound;
    std::pair<int,int>LayBound;
    int _minCol,_maxCol,_minRow,_maxRow,_minLay,_maxLay;
    int flexRow = 0,flexCol = 0,flexLay = 0;
    bool init = false;
};




#endif
