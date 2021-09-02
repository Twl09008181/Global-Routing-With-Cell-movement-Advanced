#include "../header/RoutingSchedule.hpp"



extern std::chrono::duration<double, std::milli> RoutingTime;
//Generic Routing Schedule
//if routing success, add demand and save to infos&RipId. Caller must call Accept or Rejcet itself.
//if routing failed , recover oldnet. 
bool RoutingSchedule(Graph*graph,int netid,std::vector<ReroutInfo>&infos,std::vector<int>&RipId,int defaultLayer,ReroutInfo**overflowNet)
{

    bool routingsuccess = true;
    //-------------------------------------Mode------------------------------------
    bool overflowmode = (overflowNet==nullptr)? false:true;
    //-------------------------------------RipUP----------------------------------
    auto oldnet = graph->getNetGrids(netid);
    if(oldnet->isFixed()){
        if(overflowNet){std::cout<<netid<<"already solved\n";delete *overflowNet;*overflowNet = nullptr;return true;}//overflow mode and fixed(it was already solved by other overflow net.)
        if(!oldnet->recover_mode)
            std::cerr<<"warning,net :"<<netid<<" is_fixed = true \n";
        return false; //can't Routing this net.
    }


    RipUpNet(graph,oldnet);
    oldnet->set_fixed(true);//每次拆掉就fixed,直到accept or rejct or failed.

    //-------------------------------------Routing---------------------------------
    #include <chrono>
    auto t1 = std::chrono::high_resolution_clock::now();
    TwoPinNets twopins = twoPinsGen(graph->getNet(netid),defaultLayer);
    std::pair<ReroutInfo,bool> result = Reroute(graph,netid,twopins,overflowmode);
    auto t2 = std::chrono::high_resolution_clock::now();
    RoutingTime+=t2-t1;
    //-------------------------------------Adding or Recover---------------------------------
    if(result.second)//can connect all twopin-nets
    {
        if(result.first.netgrids->isOverflow())//but overflow
        {
            AddingNet(graph,oldnet);//recover 
            **overflowNet = std::move(result.first);//saving overflowNet information.
            routingsuccess = false;
        }
        else{//update infos/RipId
            AddingNet(graph,result.first.netgrids);
            infos.push_back(result.first);
            RipId.push_back(netid);
        }
    }

    else
    {
        AddingNet(graph,oldnet);//recover oldnet demand
        oldnet->set_fixed(false);
        routingsuccess = false;
        // failed sometimes caused by routing direction  or bounding Box Region.
        // this lib do not process this situation , so delete and set nullptr.
        if(result.first.netgrids->isOverflow())
        {
            delete *overflowNet;
            *overflowNet = nullptr;
        }
        delete result.first.netgrids;
        delete result.first.nettree;
    }

    return routingsuccess;
}




bool OverflowProcess(Graph*graph,NetGrids*overflownet,std::vector<ReroutInfo>&infos,std::vector<int>&RipId)
{

    //Get overflow Ggrids
    std::unordered_map<Ggrid*,bool>overflowGrids;

    for(auto grid_info:overflownet->grids)
    {
        if(grid_info.first->is_overflow())
            overflowGrids.insert({grid_info.first,true});
    }
    
    //Get overlap netid
    std::unordered_map<int,std::set<Ggrid*>>overlapNets;//netid,overlap grids

    for(auto g:overflowGrids)
    {
        for(auto nid:g.first->Netids)
        {
            auto s = overlapNets.find(nid);
            if(s==overlapNets.end())
            {
                std::set<Ggrid*>new_s;new_s.insert(g.first);
                overlapNets.insert({nid,std::move(new_s)});
            }
            else{
                s->second.insert(g.first);
            }
        }
    }

    //sort by overlap grid num
    std::vector<std::pair<int,std::set<Ggrid*>>>netids;netids.reserve(overlapNets.size());
    for(auto &nid:overlapNets){netids.push_back(std::move(nid));}
    auto cmp = [](std::pair<int,std::set<Ggrid*>>&n1,std::pair<int,std::set<Ggrid*>>&n2){return n1.second.size() > n2.second.size();};
    std::sort(netids.begin(),netids.end(),cmp);



    //find top layer
    int toplay = 3;
    float rate = 1.0;
    for(int i = 3;i<=graph->LayerNum();i++)
    {
        auto uti = graph->lay_uti(i);
        float r = float(uti.first)/uti.second;
        
        if(r < rate-0.2)
        {
            toplay = i;
            rate = r;
        }
    }

 
    //solving overflow
    int idx = 0;
    int count = 0;
    int trylimit  = 20;
    std::vector<ReroutInfo>of_infos;std::vector<int>of_RipId;
    while(!overflowGrids.empty()&&idx<netids.size()&&count<=trylimit)
    {

        int nid = netids.at(idx).first;idx++;
        auto net = graph->getNetGrids(nid);
        if(net->isFixed())continue;
        if(net->passScore > overflownet->passScore)continue;
        count++;
        net->recover_mode = true;
        if(RoutingSchedule(graph,nid,of_infos,of_RipId,toplay))//using recover mode
        {
            for(auto g:netids.at(idx-1).second)
            {
                overflowGrids.erase(g);
            }
        }
    }

    
    if(!overflowGrids.empty())//solving failed --- recover the success net
    {
        for(auto nid:of_RipId)
        {
            graph->getNetGrids(nid)->recover_mode = true;
        }
        Reject(graph,of_infos,of_RipId);
    }else{//solving success
        for(auto info:of_infos)
            infos.push_back(info);
        for(auto id:of_RipId)
            RipId.push_back(id);
    }
    return overflowGrids.empty();
}

extern std::chrono::duration<double, std::milli> OverFlowProcessTime;
bool overFlowRouting(Graph*graph,int Netid,std::vector<ReroutInfo>&infos,std::vector<int>&RipId,int defaultLayer,ReroutInfo**)
{
    ReroutInfo* overflowNet = new ReroutInfo;
    float sc = graph->score;
    bool success = true;

    bool r = RoutingSchedule(graph,Netid,infos,RipId,defaultLayer,&overflowNet);
    if(!r)//failed
    {   
        if(!overflowNet)//not caused by overflow
        {
            return false;  //bounding box limit.
        }
        auto oldnet = graph->getNetGrids(Netid);

        RipUpNet(graph,oldnet);
        AddingNet(graph,overflowNet->netgrids);//force add
        
        auto t1 = std::chrono::high_resolution_clock::now();
        if(!OverflowProcess(graph,overflowNet->netgrids,infos,RipId))//failed
        {
    
            RipUpNet(graph,overflowNet->netgrids);
            AddingNet(graph,graph->getNetGrids(Netid));
            oldnet->set_fixed(false);
            delete overflowNet->netgrids;
            delete overflowNet->nettree;
            success = false;
        }else{//if success 

            infos.push_back(*overflowNet);
            RipId.push_back(Netid);
        } 
        auto t2 = std::chrono::high_resolution_clock::now();
        OverFlowProcessTime+=t2-t1;
    }
    else{//success
        if(graph->getNetGrids(Netid)->isOverflow())
        {
            std::cerr<<"error overflow !\n";
        }     
    }
    return success;
}


void Reject(Graph*graph,std::vector<ReroutInfo>&info,std::vector<int>&AlreadyRipUp)
{
    for(auto reroute:info)
    {
        RipUpNet(graph,reroute.netgrids);    //RipUP - Reroute result
        delete reroute.netgrids;
        delete reroute.nettree;
    }
    //Recover origin
    for(auto netId:AlreadyRipUp)
    {
        AddingNet(graph,graph->getNetGrids(netId));
        graph->getNetGrids(netId)->set_fixed(false);//release
    }
    info.clear();
    AlreadyRipUp.clear();
}

void Accept(Graph*graph,std::vector<ReroutInfo>&info)
{
    for(auto reroute:info)
    {
        int netId = reroute.netgrids->NetId;
        graph->updateNetGrids(netId,reroute.netgrids);
        graph->updateTree(netId,reroute.nettree);
    }

    info.clear();
}


std::vector<netinfo> getNetlist(Graph*graph)//sort by  wl - hpwl
{
    std::vector<netinfo>nets;nets.resize(graph->Nets.size());
    for(int i = 1;i<=graph->Nets.size();i++)
    {
        nets.at(i-1).netId = i;
        nets.at(i-1).hpwl = HPWL(&graph->getNet(i));
    }
    for(int i = 1;i<=graph->Nets.size();i++)
    {
        nets.at(i-1).wl = graph->getNetGrids(i)->wl(); 
    }
    auto cmp = [](const netinfo&n1,const netinfo&n2)
    {
        return n1.wl-n1.hpwl > n2.wl-n2.hpwl;//Bug Fixed , update n2.hpwl
    };
    std::sort(nets.begin(),nets.end(),cmp);

    // for(auto n:nets)
    // {
    //     std::cout<<n.hpwl<<" "<<n.wl<<" "<<n.wl-n.hpwl<<"\n";
    // }

    return nets;
}


#include <stdio.h>
#include <stdlib.h>
#include <time.h>

bool change_state(int cost1,int cost2,float temperature)
{
    srand( time(NULL) );
    double x = (double) rand() / (RAND_MAX + 1.0);
    int delta_cost = cost2-cost1;
   
    return x < std::exp(-delta_cost/temperature);//t越小,exp值越小,越不可能跳,delta越大,越不可能跳
}

extern int failedCount;
extern int RejectCount;
extern int AcceptCount;
extern int overflowSolved;
//BatchRoute
void BatchRoute(Graph*graph,std::vector<netinfo>&netlist,int start,int _end,routing_callback _callback,int batchsize,int default_layer)
{
    float sc = graph->score;
    for(int idx = start; idx < _end; idx += batchsize)
    {
        std::vector<ReroutInfo>infos;std::vector<int>RipId;infos.reserve(batchsize);RipId.reserve(batchsize);
        int s = idx;
        int e = min((idx+batchsize),_end);
        for(int j = s;j<e;j++){
            int nid = netlist.at(j).netId;
            _callback(graph,nid,infos,RipId,default_layer,nullptr);
        }
        if(graph->score < sc||change_state(sc,graph->score,netlist.size()-idx)){
            Accept(graph,infos);AcceptCount++;
            sc = graph->score;
        }else{
            Reject(graph,infos,RipId);RejectCount++;
        }
    }
}






extern bool t2t;
// //Route All Accept or Reject
void RouteAAoR(Graph*graph,std::vector<netinfo>&netlist,CellInst*movCell)
{
    std::vector<int>blkgLayer;
    for(auto b:movCell->mCell->blkgs){blkgLayer.push_back(b.second.first);}
    //blkgLayer must check again , because blkg metal is different from net pin metal 
    //so net pin all success can't guarantee the overflow caused by blkg is solved.
    auto blkgCheck = [](Graph*graph,CellInst*movCell,std::vector<int>&blkgLayer)
    {
        for(auto l:blkgLayer)
            if((*graph)(movCell->row,movCell->col,l).is_overflow())return false;
        return true;
    };

    float sc = graph->score;
    std::vector<ReroutInfo>infos;
    std::vector<int>RipId;
    infos.reserve(netlist.size());RipId.reserve(netlist.size());
    
    std::vector<int>failed;
    
    // t2t = true;
    for(int j = 0;j<netlist.size();j++){
        int nid = netlist.at(j).netId;
        graph->getNetGrids(nid)->recover_mode = true;
        if(!RoutingSchedule(graph,nid,infos,RipId))
        {
            failed.push_back(nid);
        }
    }

    bool success = true;
    // t2t = false;
    for(auto nid:failed)
    {
        graph->getNetGrids(nid)->recover_mode = true;
        if(!overFlowRouting(graph,nid,infos,RipId))
        {
            success = false;
            break;
        }
    }
    
    if(blkgCheck(graph,movCell,blkgLayer)&&success&&(graph->score < sc)){
    // if(success&&(graph->score < sc)){
        Accept(graph,infos);
        sc = graph->score;
        if(movCell)
        {
            movCell->originalRow = movCell->row;
            movCell->originalCol = movCell->col;
        }
    
    }else{
        if(movCell)
        {
            graph->removeCellsBlkg(movCell);
            movCell->row = movCell->originalRow;
            movCell->col = movCell->originalCol;
        }
        Reject(graph,infos,RipId);
        if(movCell){
            graph->insertCellsBlkg(movCell);
        }
    }
}



//Route Single Accept or Reject 
void Route(Graph*graph,std::vector<netinfo>&netlist)
{
    float sc = graph->score;
    
    std::vector<ReroutInfo>infos;
    std::vector<int>RipId;

    infos.reserve(netlist.size());RipId.reserve(netlist.size());

    std::vector<int>failed;//using overflow Routing latter

   

    // t2t = true;
    for(int j = 0;j<netlist.size();j++){
        int nid = netlist.at(j).netId;
        if(!RoutingSchedule(graph,nid,infos,RipId))
        {
            failed.push_back(nid);
            
        }
        else{
            if(graph->score < sc){
                Accept(graph,infos);
                sc = graph->score;AcceptCount++;
                
            }
            else{
                Reject(graph,infos,RipId);RejectCount++;
            }
            Accept(graph,infos);
        }
  
    }

    // t2t = false;
    for(auto nid:failed)
    {
        if(overFlowRouting(graph,nid,infos,RipId))
        {
            if(graph->score < sc){
                Accept(graph,infos);
                sc = graph->score;AcceptCount++;
            }
            else{
                Reject(graph,infos,RipId);RejectCount++;
            }
            overflowSolved++;
        }
        else{
            failedCount++;
        }

    }
}
