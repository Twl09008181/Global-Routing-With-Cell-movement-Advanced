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

bool t2t =false;//---------------------------------------t2t or mz-------------------------------------
int failedCount;
int RejectCount;
int AcceptCount;
int overflowSolved;
int main(int argc, char** argv)
{
    auto t1 = std::chrono::high_resolution_clock::now();
    auto t2 = std::chrono::high_resolution_clock::now();
    readLUT();
    if(argc!=2){
        std::cerr<<"Wrong parameters!"<<std::endl;
        return -1;
    }
    std::string path = "./benchmark/";
    std::string fileName = argv[1];
    
    Init(path,fileName);    
    strtable.init(graph);
    origin = graph->score;
    

    auto netlist = getNetlist(graph);
    Route(graph,netlist);
    std::cout<<"only rout score:"<<origin-graph->score<<"\n";
 
    t2t = true;
    // countdmd(graph);
    int num = 3;
    while(num--){
        RoutingWithCellMoving(graph);
        std::cout<<"move : score:"<<origin-graph->score<<"\n";
        auto netlist = getNetlist(graph);
        Route(graph,netlist);
        std::cout<<"only rout score:"<<origin-graph->score<<"\n";
    }
    // // countdmd(graph);
    OutPut(graph,fileName);

    // std::cout<<"seed:"<<seed<<"\n";
    std::cout<<"final score:"<<origin-graph->score<<"\n";
    t2 = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> t3(t2-t1);
    std::cout<<"total : "<<t3.count()/1000<<" s \n";
    std::cout<<"routing failed count:"<<failedCount<<"\n";
    std::cout<<"overflowSolved count:"<<overflowSolved<<"\n";
    std::cout<<"Accept rate:"<<100*float(AcceptCount)/(AcceptCount+RejectCount)<<"%\n";

    std::cout<<"Routing time:"<<RoutingTime.count()/1000<<"s\n";
    // std::cout<<"pins time:"<<pinsTime.count()/1000<<"s\n";
    // std::cout<<"final score:"<<origin-graph->score<<"\n";
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
