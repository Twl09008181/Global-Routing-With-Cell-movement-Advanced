#include "header/graph.hpp"
#include "header/Routing.hpp"
#include "header/TwoPinNet.hpp"
#include "header/analysis.hpp"
#include "header/RoutingSchedule.hpp"
// #include <Windows.h>
#include<unistd.h>
#include <time.h>
#include <algorithm>
#include <thread>
Graph* graph = nullptr;
std::string _fileName;
void timerStop()
{
    sleep(3600);
    std::cout<<"times up end the program\n";
    exit(1);
}
void Init(std::string fileName){graph = new Graph(fileName);}
void OutPut(Graph*graph,std::string fileName);
void RoutingWithCellMoving(Graph*graph);

#include <chrono>


table strtable;
float origin;
bool t2t = true;//---------------------------------------t2t or mz-------------------------------------


double temperature = pow(10,0);   //for hpwl check  
double temperature2 = pow(10,10);  // for routing score result check (RoutingSchedule.hpp)
double temp2decadeR = 0.995;

std::chrono::high_resolution_clock::time_point lastAcc;
std::chrono::high_resolution_clock::time_point startTime;
int main(int argc, char** argv)
{   
    
    std::thread timerThread(timerStop);

    lastAcc  = std::chrono::high_resolution_clock::now();
    startTime = std::chrono::high_resolution_clock::now();

    if(argc!=2){
        std::cerr<<"Wrong parameters!"<<std::endl;
        return -1;
    }
    _fileName = argv[1];
    //---------------------------------------init-----------------------------------------------
    readLUT();
    Init(_fileName);    
    strtable.init(graph);
    origin = graph->score;
    
    //-------------------------------------first route-----------------------------------------
    auto netlist = getNetlist(graph);
    t2t = true;//using t2t mode
    Route(graph,netlist);
    OutPut(graph,_fileName);
    lastAcc = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double,std::milli>FRT = lastAcc-startTime;;
    std::cout<<"first rout score:"<<origin-graph->score<<"  spend:  "<<FRT.count()/1000<<"s\n";
    //-------------------------------------first route-----------------------------------------


    
    
    while(1){
        RoutingWithCellMoving(graph);
        std::cout<<"move : score:"<<origin-graph->score<<"\n";
        OutPut(graph,_fileName);
        lastAcc = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double, std::milli> dur(lastAcc-startTime);
        std::cout<<"time: "<<dur.count()/1000<<" s \n";
    }
    OutPut(graph,_fileName);
    std::cout<<"final score:"<<origin-graph->score<<"\n";
    delete graph;
    timerThread.join();
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

        //Hpwl comarasion
        int oldHpwl = 0;int newHpwl = 0;
        int r = movCell->row;int c = movCell->col;
        movCell->row = movCell->originalRow;
        movCell->col = movCell->originalCol;
        for(auto n:Nets){oldHpwl+=HPWL(n.first);}
        movCell->row = r;
        movCell->col = c;
        for(auto n:Nets){newHpwl+=HPWL(n.first);}

        int totalWl = 0;
        for(auto n:Nets)
        {
            int nid = std::stoi(n.first->netName.substr(1,-1));
            int hpwl = HPWL(n.first);//new hpwl
            int wl = graph->getNetGrids(nid)->wl();//old wl
            totalWl+=wl;
            Netsid.push_back({nid,hpwl,wl});
        }


        if(change_state(pow(oldHpwl,2),pow(newHpwl,2),temperature))
            RouteAAoR(graph,Netsid,movCell);
        else{
            graph->removeCellsBlkg(movCell);
            movCell->row = movCell->originalRow;
            movCell->col = movCell->originalCol;
            graph->insertCellsBlkg(movCell);
        }

        if(movCell->row == movCell->initRow && movCell->col == movCell->initCol){
			graph->moved_cells.erase(movCell);
		}
        temperature2*=temp2decadeR;
    }
    

}
