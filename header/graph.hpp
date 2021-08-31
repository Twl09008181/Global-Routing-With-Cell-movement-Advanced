#ifndef _GRAPH_HPP_
#define _GRAPH_HPP_

#include <string>
#include <fstream>
#include <utility>
#include <cmath>
//---------------------------------------------dataStructures---------------------------------------------------
#include <unordered_map>
#include "data_structure.hpp"
struct tree;
struct node;
struct NetGrids;
class Graph{
public:
    Graph(std::string fileName);
    void parser(std::string fileName);
    ~Graph();


//---------------------------------------------General using--------------------------------------------------------
    Ggrid& operator()(int row,int col,int lay){
        if(row > RowEnd)
        {
            std::cerr<<"Ggrid accessing Error!!!,Input : row must <= RowEnd :\n"<<" row = "<<row<<" RowEnd = "<<RowEnd<<"\n";
            exit(1);
        }
        if(col > ColEnd)
        {
            std::cerr<<"Ggrid accessing Error!!!,Input : col must <= ColEnd :\n"<<" col = "<<col<<" ColEnd = "<<ColEnd<<"\n";
            exit(1);
        }
        if(lay > Layers.size())
        {
            std::cerr<<"Ggrid accessing Error!!!,Input : lay must <= LayNum :\n"<<" lay = "<<lay<<" LayNum = "<<Layers.size()<<"\n";
            exit(1);
        }
        if(row <RowBegin)
        {
            std::cerr<<"Ggrid accessing Error!!!,Input : row must >= RowBegin :\n"<<" row = "<<row<<" RowBegin = "<<RowBegin<<"\n";
            exit(1);
        }
        if(col < ColBegin)
        {
            std::cerr<<"Ggrid accessing Error!!!,Input : col must >= ColBegin :\n"<<" col = "<<col<<" ColBegin = "<<ColBegin<<"\n";
            exit(1);
        }
        if(lay < 1)
        {
            std::cerr<<"Ggrid accessing Error!!!,Input : lay must >= 1 :\n"<<" lay = "<<lay<<"\n";
            exit(1);
        }
        return Ggrids.at(lay-1).at(row-RowBegin).at(col-ColBegin);
    }

    int LayerNum()const{return Layers.size();}
    std::pair<int,int>RowBound()const{return {RowBegin,RowEnd};}
    std::pair<int,int>ColBound()const{return {ColBegin,ColEnd};}
    Net& getNet(int NetId){
        if(NetId<1||NetId>Nets.size())
        {
            std::cout<<"Net& getNet(int NetId) input Error: 1<=NetId<="<<Nets.size()<<"\n";
            std::cout<<"netID:"<<NetId<<"\n";
            exit(1);
        }
        std::string Key = "N" + std::to_string(NetId);
        return *Nets[Key];
    }
    tree* getTree(int NetId)
    {
        if(NetId<1||NetId>Nets.size())
        {
            std::cout<<"tree* getTree(int NetId) input Error: 1<=NetId<="<<Nets.size()<<"\n";
            exit(1);
        }
        return routingTree.at(NetId-1);
    }
    NetGrids* getNetGrids(int NetId)
    {
        if(NetId<1||NetId>Nets.size())
        {
            std::cout<<"NetGrids* getNetGrids(int NetId) input Error: 1<=NetId<="<<Nets.size()<<"\n";
            exit(1);
        }
        return netGrids.at(NetId-1);
    }
    void updateTree(int NetId,tree*t);
    void updateNetGrids(int NetId,NetGrids*netgrid);


    Layer& getLay(int Lay){
        return Layers.at(Lay-1);
    }
	void showEffectedNetSize();
	std::pair<std::string,CellInst*>cellMoving();
	std::vector< std::pair<std::string,CellInst*>> cellSwapping();
	void placementInit();
	void placementInit_Swap();
    bool removeCellsBlkg(CellInst* cell);
    bool insertCellsBlkg(CellInst* cell);

	double congest_value(int, int, int);
	void show_cell_pos();

    std::pair<int,int>&lay_uti(int Lay)//first : demand of layer , second : capacity of layer
    {
        if(Lay<1||Lay>LayerNum())
        {
            std::cerr<<"error input in lay_uti(int Lay)!\n";
            exit(1);
        }
        return utilizations.at(Lay-1);
    }
//--------------------------------------------Data Mmeber------------------------------------------------------------
    std::unordered_map<std::string,MasterCell*>mCell;
    std::unordered_map<std::string,CellInst*>CellInsts;
    std::unordered_map<std::string,Net*>Nets;
    std::vector<std::vector<std::pair<int,int>>>voltageAreas;
    std::vector<Layer>Layers;
    
	std::unordered_set<CellInst*> moved_cells;
	std::unordered_map<int, std::unordered_set<int>> voltage_include;

    
    
private:
    using Ggrid1D = std::vector<Ggrid>;
    using Ggrid2D = std::vector<Ggrid1D>;
    using Ggrid3D = std::vector<Ggrid2D>;
    Ggrid3D Ggrids;
    std::vector<std::pair<int,int>>utilizations;
    int RowBegin,ColBegin;
    int RowEnd,ColEnd;

    int movement_stage;
	std::priority_queue< std::tuple<int, int, int, int, int>> candiPq;
	std::priority_queue< std::tuple<int, int, int, int>> swappingCandiPq;



//---------------------------------------------RoutingTree-------------------------------------------------------------
public:
    int MAX_Cell_MOVE;
    std::vector<tree*>routingTree;
    std::vector<NetGrids*>netGrids;
    float score = 0;
};



#endif
