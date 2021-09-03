#ifndef R_SCHLER
#define R_SCHLER

#include "graph.hpp"
#include"Routing.hpp"
#include "analysis.hpp"
#include <vector>


struct netinfo{
    int netId;
    int hpwl;
    int wl;
};
std::vector<netinfo> getNetlist(Graph*graph);//sort by  wl - hpwl

void Reject(Graph*graph,std::vector<ReroutInfo>&info,std::vector<int>&AlreadyRipUp);
void Accept(Graph*graph,std::vector<ReroutInfo>&info);

bool RoutingSchedule(Graph*graph,int netid,std::vector<ReroutInfo>&infos,std::vector<int>&RipId,int defaultLayer=0,ReroutInfo**overflowNet=nullptr,bool recover = true);
bool overFlowRouting(Graph*graph,int Netid,std::vector<ReroutInfo>&infos,std::vector<int>&RipId,int defaultLayer=0,ReroutInfo**overflowNet=nullptr);
using routing_callback = decltype(RoutingSchedule)*;

void BatchRoute(Graph*graph,std::vector<netinfo>&netlist,int start,int _end,routing_callback _callback,int batchsize=1,int default_layer=0);
bool RouteAAoR(Graph*graph,std::vector<netinfo>&netlist,CellInst*c = nullptr,bool recover = true);//Route "All" Accept or Reject , can be used as batch route.
void Route(Graph*graph,std::vector<netinfo>&netlist);//Route "Single" Accept or Reject 
//simple example
// void OnlyRouting(Graph*graph,int batchSize,bool overflow,float topPercent)
// {
//     float sc = graph->score;
//     std::vector<netinfo> netlist = getNetlist(graph);//get netList
//     //-----------------overflow allowed----------------------------
//     if(overflow){
//         routing(graph,netlist,0,netlist.size()*topPercent,overFlowRouting,batchSize);
//     }
//     //------------------------------------------------------------
//     routing(graph,netlist,0,netlist.size(),overFlowRouting,batchSize);
// }


inline bool change_state(float cost1,float cost2,float temperature)
{

    float c = cost1-cost2;//more bigger, more good 
    srand( time(NULL) );
    double x = (double) rand() / (RAND_MAX + 1.0);

    double successProb = 1 /(1+exp(-c/temperature));
    return successProb > x;
}
#endif
