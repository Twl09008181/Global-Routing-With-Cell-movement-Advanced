#include "header/graph.hpp"
#include "header/Routing.hpp"
#include "header/TwoPinNet.hpp"
#include "header/analysis.hpp"
#include "header/RoutingSchedule.hpp"

#include <time.h>
#include <thread>
#include <algorithm>
Graph* graph = nullptr;


void Init( std::string path,std::string fileName){graph = new Graph(path+fileName);}
void OutPut(Graph*graph,std::string fileName);
// void OnlyRouting(Graph*graph,int batchSize = 1,bool overflow = false,float topPercent = 0);
void RoutingWithCellMoving(Graph*graph);

#include <chrono>
std::chrono::duration<double, std::milli> IN;
std::chrono::duration<double, std::milli> RoutingTime;
std::chrono::duration<double, std::milli> OverFlowProcessTime;
std::chrono::duration<double, std::milli> pinsTime;

std::chrono::duration<double,std::milli>movtime;
std::chrono::duration<double,std::milli>collectTime;

table strtable;
float origin;

bool t2t = true;//---------------------------------------t2t or mz-------------------------------------
int failedCount;
int RejectCount;
int AcceptCount;
int overflowSolved;

bool firstRout = true;
int Bxflex;

std::chrono::high_resolution_clock::time_point lastAcc;
std::chrono::high_resolution_clock::time_point startTime;
int main(int argc, char** argv)
{
    lastAcc  = std::chrono::high_resolution_clock::now();
    startTime = std::chrono::high_resolution_clock::now();

    if(argc!=2){
        std::cerr<<"Wrong parameters!"<<std::endl;
        return -1;
    }
    std::string path = "./benchmark/";
    std::string fileName = argv[1];

    //---------------------------------------init-----------------------------------------------
    readLUT();
    Init(path,fileName);    
    strtable.init(graph);
    origin = graph->score;
    
    //-------------------------------------first route-----------------------------------------
    auto netlist = getNetlist(graph);
    t2t = true;//using t2t mode
    Route(graph,netlist);
    OutPut(graph,fileName);
    lastAcc = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double,std::milli>FRT = lastAcc-startTime;;
    firstRout = false;
    std::cout<<"first rout score:"<<origin-graph->score<<"  spend:  "<<FRT.count()/1000<<"s\n";
    //-------------------------------------first route-----------------------------------------


    t2t = false;//Maze routing
    int num = 100;
    while(num--){
        RoutingWithCellMoving(graph);
        std::cout<<"move : score:"<<origin-graph->score<<"\n";
        auto netlist = getNetlist(graph);
        Route(graph,netlist);
        OutPut(graph,fileName);
        lastAcc = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double, std::milli> dur(lastAcc-startTime);
        std::cout<<"time: "<<dur.count()/1000<<" s \n";
    }
    
    std::cout<<"final score:"<<origin-graph->score<<"\n";
    // std::cout<<"Accept rate:"<<100*float(AcceptCount)/(AcceptCount+RejectCount)<<"%\n";

    delete graph;
	return 0;
}

std::map<Net*,int> RelatedNets(CellInst*c)
{
    std::map<Net*,int>Nets;//有一些net重複了
    int Idx = 0;
    for(auto net:c->nets){
        if(Nets.find(net)==Nets.end())
            Nets.insert({net,Idx++});
    }
    return Nets;
}
void OutPut(Graph*graph,std::string fileName)
{

    std::vector<std::string>segments;
    std::cout<<"Routing complete !\n";
    PrintAll(graph,&segments);
    //寫成輸出檔案
    int NumRoutes = segments.size();
    fileName = fileName.substr(0,fileName.size()-4);
    fileName = fileName+"Out.txt";
    std::ofstream os{fileName};
    if(!os){
        std::cerr<<"error:file "<<fileName<<" cann't open!\n";
        exit(1);
    } 

    os<<"NumMovedCellInst "<< graph->moved_cells.size() <<"\n";
    for(auto cell:graph->moved_cells)
        os<<"CellInst "<< cell->name << " " << cell->row << " " << cell->col <<"\n";
    os<<"NumRoutes "<<NumRoutes<<"\n";

    for(auto s:segments)
    {
        os<<s<<"\n";
    }
    std::cout<<"saving done!\n";

    os.close();
}


void RoutingWithCellMoving(Graph*graph)
{
    graph->placementInit();

    std::pair<std::string,CellInst*>movcellPair;

    int mov = 0;
    while((movcellPair=graph->cellMoving()).second)
    {
        CellInst* movCell = movcellPair.second;
        mov++;
        graph->moved_cells.insert(movCell);
        if(graph->moved_cells.size()>graph->MAX_Cell_MOVE)
        {
            graph->removeCellsBlkg(movCell);
            movCell->row = movCell->originalRow;
            movCell->col = movCell->originalCol;
            graph->insertCellsBlkg(movCell);
			if(movCell->row == movCell->initRow && movCell->col == movCell->initCol){
				graph->moved_cells.erase(movCell);
			}
            continue;
        }


        std::map<Net*,int>Nets = RelatedNets(movCell);
        std::vector<netinfo>Netsid;Netsid.reserve(Nets.size());

        for(auto n:Nets)
        {
            int nid = std::stoi(n.first->netName.substr(1,-1));
            int hpwl = HPWL(n.first);
            int wl = graph->getNetGrids(nid)->wl();
            Netsid.push_back({nid,hpwl,wl});
        }

        RouteAAoR(graph,Netsid,movCell);
       
        if(movCell->row == movCell->initRow && movCell->col == movCell->initCol){
			graph->moved_cells.erase(movCell);
		}
        
    }


}
