#ifndef _DATA_STRUCTURE_HPP_
#define _DATA_STRUCTURE_HPP_
// #define PARSER_TEST

//#include "bits/stdc++.h"
#include <utility>
#include <unordered_map>
#include <set>
#include <map>
#include <float.h>
#include <unordered_set>
#include <string>
#include <fstream>
#include <algorithm>
#include <iostream>
#include <vector>
#include <queue>
#include <functional>
extern "C" {
    #include "../flute-3.1/flute.h"
}

std::vector<std::string> split(const std::string&str,char symbol,int l,int r,int num);

struct Layer{
    Layer(const std::string& RoutDir = "H",int defaultSupply = 0,float pf = 0)
        : powerFactor{pf},supply{defaultSupply},horizontal{(RoutDir=="H")}{}
    bool horizontal;//<RoutingDir>
    int supply;//<defaultSupplyOfOneGGrid>
    float powerFactor;//<powerFactor>
};


struct MasterCell
{
    MasterCell(std::ifstream&is,std::unordered_map<std::string,MasterCell*>&mCell);
    using Blkg = std::pair<int,int>;//(Layer index,demand)
    using Pin = int;//Layer
    std::unordered_map<std::string,Pin>pins;//PinName : Pin
    std::unordered_map<std::string,Blkg>blkgs;//BlkgName : Blkg
	
	void sumUpBlkg();
	int blkgSum;
};


struct Net;
struct CellInst{
    CellInst(std::ifstream&is,std::unordered_map<std::string,MasterCell*>&mCells,std::unordered_map<std::string,CellInst*>&CellInsts);
    MasterCell* mCell;
    int row;//<gGridRowIdx>
    int col;//<gGridColIdx>

	int originalRow, originalCol;

    bool Movable;//<movableCstr>
    //VoltageArea* vArea;//null if no VoltageArea
    int vArea = -1;//" from 0 to graph.cpp:voltageAreas.size()-1 "


	int demandrequire;
	std::vector<Net*> nets;
	std::vector<int> optimalRegion;

	void fixCell();
	void updateOptimalRegion();
	bool inOptimalRegion(int row, int col);
};


struct Ggrid{
    Ggrid(int c) : capacity(c), demand(0) {}
    int get_remaining(void)const {return capacity - demand;}
    float congestion_rate()const{
        if(capacity==0)return 1; //以免 / 0 error.
        return (float)demand/capacity;
    }
    void add_demand(){demand+=1;}
    void delete_demand(){using std::max;demand = max(demand-1,0);}
//----------------------------------Data Member----------------------------------------------
    int capacity;
    int demand;
    int row,col,lay;

//---------------------------------RoutingFlag-------------------------------------------
    Net* enrollNet = nullptr;//for demand interface
    bool isTarget = false;//for tree2tree
};

struct Net{
    std::string netName;//<netName>
    Net(std::ifstream&is,std::unordered_map<std::string,CellInst*>&CellInsts,std::unordered_map<std::string,Net*>&Nets);
    //會用到的..................................................................................
    int minLayer;//<minRoutingLayConstraint>
    float weight;//<weight>
    using PIN = std::pair<CellInst*,std::string>;
    //記下CellInst*是為了以後移動可以得到更新的座標 x,y
    //std::string代表Pin的name,用來CellInst內查找Pin
    std::vector<PIN> net_pins;
    enum class  state {RipUpinit,Adding,CanAdd,doneAdd,dontcare}; //for rip-up
    state routingState = state::CanAdd;


    bool RerouteFlag = false;
	std::vector<int> fixedBoundingBox;
	void updateFixedBoundingBox();

	int costToBox(int, int);
};


#endif
