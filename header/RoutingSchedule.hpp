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

bool RoutingSchedule(Graph*graph,int netid,std::vector<ReroutInfo>&infos,std::vector<int>&RipId,int defaultLayer=0,ReroutInfo**overflowNet=nullptr);
bool overFlowRouting(Graph*graph,int Netid,std::vector<ReroutInfo>&infos,std::vector<int>&RipId,int defaultLayer=0,ReroutInfo**overflowNet=nullptr);
using routing_callback = decltype(RoutingSchedule)*;

void routing(Graph*graph,std::vector<netinfo>&netlist,int start,int _end,routing_callback _callback,int batchsize=1,int default_layer=0);


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


#endif
