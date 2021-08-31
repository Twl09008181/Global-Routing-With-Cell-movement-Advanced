#include "../header/RoutingSchedule.hpp"




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
        if(overflowNet){delete *overflowNet;*overflowNet = nullptr;return false;}
        if(!oldnet->recover_mode)
            std::cerr<<"warning,net :"<<netid<<" is_fixed = true \n";
        return false; //can't Routing this net.
    }


    RipUpNet(graph,oldnet);
    oldnet->set_fixed(true);

    //-------------------------------------Routing---------------------------------
    TwoPinNets twopins = twoPinsGen(graph->getNet(netid),defaultLayer);
    std::pair<ReroutInfo,bool> result = Reroute(graph,netid,twopins,overflowmode);

    //-------------------------------------Adding or Recover---------------------------------

    if(result.first.netgrids->isOverflow())//failed: overflow happend
    {
        AddingNet(graph,oldnet);
        **overflowNet = std::move(result.first);
        routingsuccess = false;
        // std::cout<<oldnet->NetId<<"overflow\n";
    }
    else{ //not overflow
        if(result.second) //success
        {   
            AddingNet(graph,result.first.netgrids);
            infos.push_back(result.first);
            RipId.push_back(netid);
        }
        else{//failed : not overflow mode
            AddingNet(graph,oldnet);//recover oldnet demand
            delete result.first.netgrids;
            delete result.first.nettree;
            oldnet->set_fixed(false);
            // std::cout<<oldnet->NetId<<"failed\n";

            if(overflowNet){
                delete *overflowNet;
                *overflowNet = nullptr;
            }
            routingsuccess = false;
            if(overflowNet)
            {
                delete *overflowNet;
                *overflowNet = nullptr;
            }
        }
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



    int idx = 0;
    int trylimit = 20;
    int count = 0;

    //solving overflow
    std::vector<ReroutInfo>of_infos;std::vector<int>of_RipId;
    while(!overflowGrids.empty()&&idx<netids.size()&&count<=trylimit)
    {

        int nid = netids.at(idx).first;idx++;
        auto net = graph->getNetGrids(nid);
        if(net->isFixed())continue;
        if(net->passScore > overflownet->passScore)continue;
        count++;
        net->recover_mode = true;
        if(RoutingSchedule(graph,nid,of_infos,of_RipId))
        {
            for(auto g:netids.at(idx-1).second)
            {
                overflowGrids.erase(g);
            }
        }
    }

    
    if(!overflowGrids.empty())//solving failed --- recover the success net
    {
        Reject(graph,of_infos,of_RipId);
    }else{//solving success
        for(auto info:of_infos)
            infos.push_back(info);
        for(auto id:of_RipId)
            RipId.push_back(id);
    }
    return overflowGrids.empty();
}


bool overFlowRouting(Graph*graph,int Netid,std::vector<ReroutInfo>&infos,std::vector<int>&RipId,int defaultLayer,ReroutInfo**)
{
    ReroutInfo* overflowNet = new ReroutInfo;
    float sc = graph->score;
    bool success = true;
   
    if(!RoutingSchedule(graph,Netid,infos,RipId,0,&overflowNet))//overflow
    {   
        // std::cout<<"overflow routing failed \n";
        if(!overflowNet)
        {
            return false;  //sometimes this overflownet be reroute because other overflow net process.
        }
        auto oldnet = graph->getNetGrids(Netid);
        // std::cout<<"oldnet nid:"<<oldnet->NetId<<"\n";
        RipUpNet(graph,oldnet);
        // std::cout<<"oldnet nid:"<<oldnet->NetId<<"\n";
        // std::cout<<overflowNet->netgrids->grids.size()<<"\n";
        AddingNet(graph,overflowNet->netgrids);//force add
        
        // std::cout<<Netid<<"enter overflow process\n";
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
    }
    else{//success

        if(graph->getNetGrids(Netid)->isOverflow())
        {
            std::cerr<<"error overflow !\n";
        }     
    }
    return success;
}



void routing(Graph*graph,std::vector<netinfo>&netlist,int start,int _end,routing_callback _callback,int batchsize,int default_layer)
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
        if(graph->score <= sc){
            Accept(graph,infos);
            sc = graph->score;
        }else
            Reject(graph,infos,RipId);
    }
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
