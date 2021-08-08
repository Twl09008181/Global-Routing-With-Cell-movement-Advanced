#ifndef _GRAPH_HPP_
#define _GRAPH_HPP_

#include <string>
#include <fstream>
#include <utility>
#include <cmath>
//---------------------------------------------dataStructures---------------------------------------------------
#include <unordered_map>
#include "data_structure.hpp"

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
            exit(1);
        }
        std::string Key = "N" + std::to_string(NetId);
        return *Nets[Key];
    }
    Layer& getLay(int Lay){
        return Layers.at(Lay-1);
    }
    void DoneReroute();
    bool RipUpDemand(Net&net);
    bool RecoverDemand(Net&net);
	void PinsInit(Net&net);
    void PinsRemov(Net&net);
	void showEffectedNetSize();
	CellInst* cellMoving();
	void placementInit();
    int get_two_pins(std::vector<std::pair<Net::point,Net::point>>& two_pin_nets,Net&net);
//--------------------------------------------Data Mmeber------------------------------------------------------------
    std::unordered_map<std::string,MasterCell*>mCell;
    std::unordered_map<std::string,CellInst*>CellInsts;
    std::unordered_map<std::string,Net*>Nets;
    std::vector<std::vector<std::pair<int,int>>>voltageAreas;
    std::vector<Layer>Layers;
private:
    using Ggrid1D = std::vector<Ggrid>;
    using Ggrid2D = std::vector<Ggrid1D>;
    using Ggrid3D = std::vector<Ggrid2D>;
    Ggrid3D Ggrids;
    int MAX_Cell_MOVE;
    int RowBegin,ColBegin;
    int RowEnd,ColEnd;

	std::priority_queue< std::tuple<int, int, int>> candiPq;
};



#endif
