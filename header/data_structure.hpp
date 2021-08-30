#ifndef _DATA_STRUCTURE_HPP_
#define _DATA_STRUCTURE_HPP_
// #define PARSER_TEST

//#include "bits/stdc++.h"
#include <mutex>
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
    
    bool is_overflow()const{return capacity < demand;}
    int get_remaining(void)const //sometimes , overflow is allowed.
    {
        if(is_overflow()){return 0;}
        return capacity - demand;
    }
    void add_demand(int dmd=1,int netid = 0,bool nocheck = false){
        demand+=dmd; 
        if(demand > capacity&&!nocheck){std::cerr<<row<<","<<col<<","<<lay<<" overflow!!\n";exit(1);}
        if(netid)Netids.insert(netid);
    }
    void delete_demand(int dmd = 1,int netid=0){
        if(demand < dmd){std::cout<<"delete_demand error!! "<<row<<" "<<col<<" "<<lay<<"\n";exit(1);}
        demand-=dmd;
        if(netid)Netids.erase(netid);
    }

//----------------------------------Data Member----------------------------------------------
    int row,col,lay;
    int capacity;
    int demand;
    std::set<int>Netids;//record the nets passing this grid.
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

 

	std::vector<int> fixedBoundingBox;
	void updateFixedBoundingBox();

	int costToBox(int, int);
};


#endif
