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
void OutPut(Graph*graph,std::string fileName,const std::vector<std::string>&MovingCell);
void OnlyRouting(Graph*graph,int batchSize = 1,bool overflow = false,float topPercent = 0);


#include <chrono>
std::chrono::duration<double, std::milli> IN;
std::chrono::duration<double, std::milli> RoutingTime;
std::chrono::duration<double, std::milli> pinsTime;



table strtable;
float origin;


int main(int argc, char** argv)
{
    auto t1 = std::chrono::high_resolution_clock::now();

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
    auto t2 = std::chrono::high_resolution_clock::now();
    IN = t2-t1;

    std::cout<<"graph Init done!\n";
    std::cout<<"Init score : "<<graph->score<<"\n";
    std::cout<<"init time :"<<(IN).count()/1000<<"s \n";



    OnlyRouting(graph,1,true,0.9);

    std::cout<<"Routing complete !\n";
    std::cout<<"final score:"<<origin-graph->score<<"\n";
    OutPut(graph,fileName,{});

    t2 = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> t3(t2-t1);
    std::cout<<"total : "<<t3.count()/1000<<" s \n";
    // std::cout<<"Routing time:"<<RoutingTime.count()/1000<<"s\n";
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

void OutPut(Graph*graph,std::string fileName,const std::vector<std::string>&MovingCell)
{

    std::vector<std::string>segments;

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

    os<<"NumMovedCellInst "<<MovingCell.size()<<"\n";
    for(auto cell:MovingCell)
        os<<"CellInst "<<cell<<"\n";
    os<<"NumRoutes "<<NumRoutes<<"\n";

    for(auto s:segments)
    {
        os<<s<<"\n";
    }
    std::cout<<"saving done!\n";

    os.close();
}


//先做一個topPercent的版本
void OnlyRouting(Graph*graph,int batchSize,bool overflow,float topPercent)
{
    float sc = graph->score;
    std::vector<netinfo> netlist = getNetlist(graph);//get netList
    //-----------------overflow allowed----------------------------
    if(overflow){
        routing(graph,netlist,0,netlist.size()*topPercent,overFlowRouting,batchSize);
    }
    //------------------------------------------------------------
    routing(graph,netlist,0,netlist.size(),RoutingSchedule,batchSize);
}
