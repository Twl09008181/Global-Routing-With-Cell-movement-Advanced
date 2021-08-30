#include "header/graph.hpp"
#include "header/Routing.hpp"
#include "header/TwoPinNet.hpp"
#include "header/analysis.hpp"

#include <time.h>
#include <thread>
#include <algorithm>
Graph* graph = nullptr;


void Init( std::string path,std::string fileName){graph = new Graph(path+fileName);}
// void OnlyRouting(Graph*graph,std::string fileName,const std::vector<std::string> &cellinfo);
void OutPut(Graph*graph,std::string fileName,const std::vector<std::string>&MovingCell);
// void RoutingWithCellMOV(Graph*graph,std::string fileName,std::vector<std::string>&MovingCell,bool Ripall=false);
bool RoutingSchedule(Graph*graph,int netid,std::vector<ReroutInfo>&infos,std::vector<int>&RipId,int defaultLayer=0,ReroutInfo**overflowNet=nullptr);
struct netinfo{
    int netId;
    int hpwl;
    int wl;
};
std::vector<netinfo> getNetlist(Graph*graph);//sort by  wl - hpwl
float routing(Graph*graph,int batchsize);


void OnlyRouting(Graph*graph,bool overflow = false,float topPercent = 0);

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


    
 


    //--------------------------------------testing------------------------------------------
    //batch size + step = new batch size
    //if this run , score is less than last run , moving opposite direction

    OnlyRouting(graph,true,0.01);
    // OnlyRouting(graph,false);

    // OnlyRouting(graph,true,0.01);
    // routing(graph,1);
    //--------------------------------------testing------------------------------------------

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




//用於一次Rip up多條後 Reject的處理
//info : 已經Reroute成功的
//AlreadyRipUP: 已經被RipUp過的netId
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










void OnlyRouting(Graph*graph,std::string fileName,const std::vector<std::string> &cellinfo)
{
    

    
    int BestSc = graph->score;
    int success = 0;
    time_t t1,t2,t3;
    t1 = time(NULL);
    t2 = time(NULL);
    int interval = 60;
    int count = 0;
    
    
    std::vector<netinfo> netlist = getNetlist(graph);//get netList


    
    for(auto n :netlist)
    {
        int i = n.netId;
        count++;
        //-------------------------------------------------Routing Init---------------------------------------------------
        bool movingsuccess = true;
        std::vector<ReroutInfo>infos;//each time a routing success , insert nettree/netgrids.
        std::vector<int>RipId;//each time rip-up , need insert netId to RipId.
        //-------------------------------------------------Routing Init---------------------------------------------------

        //-------------------------------------------------RipUP----------------------------------------------------------
        auto net = graph->getNetGrids(i);
        RipUpNet(graph,net);RipId.push_back(i);
        //-------------------------------------------------RipUP----------------------------------------------------------

        //-------------------------------------------------Routing----------------------------------------------------------
        auto t11 = std::chrono::high_resolution_clock::now();
        TwoPinNets twopins;
        twopins = twoPinsGen(graph->getNet(i));
        auto t12 = std::chrono::high_resolution_clock::now();
        pinsTime+=t12-t11;

        t11 = std::chrono::high_resolution_clock::now();
        std::pair<ReroutInfo,bool> result = Reroute(graph,net->NetId,twopins);
        t12 = std::chrono::high_resolution_clock::now();

        RoutingTime+=t12-t11;

        if(result.second==false)//failed
        {
            movingsuccess = false;
        }
        else{//success
            AddingNet(graph,result.first.netgrids);
            infos.push_back(result.first);
        }
        
        //-------------------------------------------------Routing----------------------------------------------------------


        //-------------------------------------------------Accept or Reject-------------------------------------------------

     
        if(movingsuccess)
        {
            //if(graph->score < BestSc)//改成SA
            if((graph->score < BestSc))
            {
                Accept(graph,infos);
                BestSc = graph->score;
                success++;
                t3 = time(NULL);
                if(t3 > t2 + interval)//1 min 
                {
                    t2 = time(NULL);
                    std::cout<<"spend "<<t3-t1<<" seconds\n";
                    std::cout<<"Best = "<<BestSc<<"\n";
                    std::cout<<"count = "<<count<<"success = "<<success<<"\n";
                    // OutPut(graph,fileName,cellinfo);
                }
            }
            else{//Reject
                Reject(graph,infos,RipId);
            }
        }
        else{//Reject
            Reject(graph,infos,RipId);
        }
   
        //-------------------------------------------------Accept or Reject-------------------------------------------------  
    }
    t3 = time(NULL);
    std::cout<<"DONE : spend "<<t3-t1<<" seconds\n";
    std::cout<<"count = "<<count<<"success = "<<success<<"\n";
    std::cout<<"score = "<<graph->score<<"\n";
    // OutPut(graph,fileName,cellinfo);
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

// void RoutingWithCellMOV(Graph*graph,std::string fileName,std::vector<std::string>&movingCellInfo,bool Ripall)
// {
//     graph->placementInit();
//     int mov = 0;
//     int success = 0;
//     std::pair<std::string,CellInst*>movcellPair;
//     float BestSc = graph->score;
//     time_t t1,t2,t3;
//     t1 = time(NULL);
//     t2 = time(NULL);
//     int interval = 60;
//     while( (movcellPair = graph->cellMoving()).second )//
//     {   
//         CellInst* movCell = movcellPair.second;
//         mov++;
//         // if(mov>graph->MAX_Cell_MOVE/20)
//         // {
//         //     movCell->row = movCell->originalRow;
//         //     movCell->col = movCell->originalCol;
//         //     graph->insertCellsBlkg(movCell);
//         //     break;
//         // }
        
//         movingCellInfo.push_back(movcellPair.first+" "+std::to_string(movCell->row)+" "+std::to_string(movCell->col));
        
//         if(movingCellInfo.size()>graph->MAX_Cell_MOVE)
//         {
//             movingCellInfo.pop_back();
//             movCell->row = movCell->originalRow;
//             movCell->col = movCell->originalCol;
//             graph->insertCellsBlkg(movCell);
//             break;
//         }

//         //-----------------------------------------------RelatedNets------------------------------------------------------
//         std::map<Net*,int>Nets = RelatedNets(movCell);

//         //-------------------------------------------------Routing Init---------------------------------------------------
//         bool movingsuccess = true;
//         std::vector<ReroutInfo>infos;infos.reserve(Nets.size());//each time a routing success , insert nettree/netgrids.
//         std::vector<int>RipId;RipId.reserve(Nets.size());//each time rip-up , need insert netId to RipId.
//         //-------------------------------------------------Routing Init---------------------------------------------------

        
//         //---------------------------------------------------RipUP----------------------------------------------------------
//         if(Ripall){
//             for(auto net:Nets)//Ripup all related
//             {
//                 int netId = std::stoi(net.first->netName.substr(1,-1));
//                 RipUpNet(graph,graph->getNetGrids(netId));RipId.push_back(netId);
                
//             }
//         }

//         for(auto net:Nets)
//         {
//             int netId = std::stoi(net.first->netName.substr(1,-1));

//             if(!Ripall){
//                 RipUpNet(graph,graph->getNetGrids(netId));RipId.push_back(netId);
//             }
//             //---------------------------------------------------RipUP----------------------------------------------------------

//             //-------------------------------------------------Routing----------------------------------------------------------
//             TwoPinNets twopins;
//             get_two_pins(twopins,*net.first);
//             std::pair<ReroutInfo,bool> result = Reroute(graph,netId,twopins);

//             if(result.second==false)//failed
//             {
//                 movingsuccess = false;
//                 break;
//             }
//             else{//success
//                 AddingNet(graph,result.first.netgrids);
//                 infos.push_back(result.first);
//             }
//             //-------------------------------------------------Routing----------------------------------------------------------
//         }
            

//         //-------------------------------------------------Accept or Reject-------------------------------------------------
//         if(movingsuccess)
//         {
//             if(graph->score < BestSc)  //Accept
//             {
//                 Accept(graph,infos);
//                 BestSc = graph->score;
//                 success++;
//                 t3 = time(NULL);
//                 if(t3 > t2 + interval)
//                 {
//                     t2 = time(NULL);
//                     std::cout<<"spend "<<t3-t1<<" seconds\n";
//                     std::cout<<"Best = "<<BestSc<<"\n";
//                     OutPut(graph,fileName,movingCellInfo);
//                 }
//             }
//             else{                     //Reject
//                 movingCellInfo.pop_back();
//                 Reject(graph,infos,RipId);
//                 movCell->row = movCell->originalRow;
//                 movCell->col = movCell->originalCol;
//                 graph->insertCellsBlkg(movCell);
//             }
//         }
//         else{                        //Reject
//             movingCellInfo.pop_back();
//             Reject(graph,infos,RipId);
//             movCell->row = movCell->originalRow;
//             movCell->col = movCell->originalCol;
//             graph->insertCellsBlkg(movCell);
//         }
//         //-------------------------------------------------Accept or Reject-------------------------------------------------
//     }
//     std::cout<<"count = "<<mov<<"\n";
//     std::cout<<"sucess = "<<success<<"\n";
//     OutPut(graph,fileName,movingCellInfo);
//     std::cout<<"score = "<<graph->score<<"\n";
//     t2 = time(NULL);
//     std::cout<<"spend "<<t2-t1<<" seconds\n";
   
// }

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
            routingsuccess = false;
        }
    }
    return routingsuccess;
}




//a simple example of routing 
float routing(Graph*graph,int batchsize)
{
    std::vector<netinfo> netlist = getNetlist(graph);//get netList
    //做一次batch再決定要不要接受,可以比較有彈性(不然一次就決定可能卡到local)

    int totalBatch = 0;
    int successnet = 0;
    int accepttime = 0;//讓他越高越好

    for(int i = 0;i<netlist.size();i+=batchsize)
    {
        float sc = graph->score;
        totalBatch++;
        //----------------------------one batch--------------------------------------
        int s = i;
        int e = min((i+batchsize),netlist.size());
        std::vector<ReroutInfo>infos;infos.reserve(batchsize);
        std::vector<int>RipId;RipId.reserve(batchsize);
        for(int j = s;j<e;j++){
            if(RoutingSchedule(graph,netlist.at(j).netId,infos,RipId))
                successnet++;
        }
        if(graph->score <= sc){
            Accept(graph,infos);
            accepttime++;
            sc = graph->score;
        }
        else
            Reject(graph,infos,RipId);
        //----------------------------one batch--------------------------------------
    }

    std::cout<<"Batch accept rate : "<<float(accepttime)/totalBatch<<"  net success rate:"<<float(successnet)/graph->Nets.size()<<"  ";
    std::cout<<"score:"<<origin-graph->score<<"\n";
    return origin-graph->score;
}

bool OverflowProcess(Graph*graph,NetGrids*overflownet)
{













    return false;//test 
}

//一次拆一條
bool overFlowRouting(Graph*graph,int Netid,std::vector<ReroutInfo>&infos,std::vector<int>&RipId)
{
    ReroutInfo* overflowNet = new ReroutInfo;
    float sc = graph->score;
    bool success = true;
    if(!RoutingSchedule(graph,Netid,infos,RipId,0,&overflowNet))//overflow
    {
        auto oldnet = graph->getNetGrids(Netid);
        RipUpNet(graph,oldnet);
        AddingNet(graph,overflowNet->netgrids);//force add

        // std::cout<<Netid<<"enter overflow process\n";
        if(!OverflowProcess(graph,overflowNet->netgrids))//failed
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




//先做一個topPercent的版本
void OnlyRouting(Graph*graph,bool overflow,float topPercent)
{
    float sc = graph->score;
    std::vector<netinfo> netlist = getNetlist(graph);//get netList

    
    int idx = 0;
    //-----------------overflow allowed----------------------------

    int count = 0;
    int success = 0;

    int total = netlist.size();
    if(overflow){
        std::vector<ReroutInfo>infos;std::vector<int>RipId;
        for(;idx < ceil(total*topPercent); idx++)
        {
            int nid = netlist.at(idx).netId;
            count++;
            if(overFlowRouting(graph,nid,infos,RipId)){
                success++;
            }
        }
        if(graph->score < sc)
        {
            Accept(graph,infos);
            sc = graph->score;
        }else{
            Reject(graph,infos,RipId);
        }
    }


    std::cout<<"success = "<<success<<" count = "<<count<<"\n";


    

    //------------------------------------------------------------
    idx = 0;
    int batchsize = 1;
    for(;idx<netlist.size();idx+=batchsize)
    {
        std::vector<ReroutInfo>infos;std::vector<int>RipId;infos.reserve(batchsize);RipId.reserve(batchsize);
        int s = idx;
        int e = min((idx+batchsize),netlist.size());
        for(int j = s;j<e;j++){
            int nid = netlist.at(j).netId;
            RoutingSchedule(graph,nid,infos,RipId);
        }
        if(graph->score <= sc){
            Accept(graph,infos);
            sc = graph->score;
        }else
            Reject(graph,infos,RipId);
    }
}
