#include "header/graph.hpp"
#include "header/Routing.hpp"
#include "header/TwoPinNet.hpp"
#include <time.h>
#include <thread>
Graph* graph = nullptr;


void Init( std::string path,std::string fileName){graph = new Graph(path+fileName);}
void OnlyRouting(Graph*graph,std::string fileName,const std::vector<std::string> &cellinfo);
void OutPut(Graph*graph,std::string fileName,const std::vector<std::string>&MovingCell);
void RoutingWithCellMOV(Graph*graph,std::string fileName,std::vector<std::string>&MovingCell,bool Ripall=false);

int main(int argc, char** argv)
{
    readLUT();
    if(argc!=2){
        std::cerr<<"Wrong parameters!"<<std::endl;
        return -1;
    }
    std::string path = "./benchmark/";
    std::string fileName = argv[1];

    Init(path,fileName);    

    std::cout<<"graph Init done!\n";
    
    
    std::vector<std::string>MovingCell;
    
    // RoutingWithCellMOV(graph,fileName,MovingCell,true);//一次拆全部相關的
    // RoutingWithCellMOV(graph,fileName,false);//一次拆一條相關的
    OnlyRouting(graph,fileName,MovingCell);//單純routing
    

    
    delete graph;
	return 0;
}




//用於一次Rip up多條後 Reject的處理
//info : 已經Reroute成功的
//AlreadyRipUP: 已經被RipUp過的netId
void Reject(Graph*graph,std::vector<ReroutInfo>&info,std::vector<int>&AlreadyRipUp)
{
    //Ripup routing result
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
    }
}

void Accept(Graph*graph,std::vector<ReroutInfo>&info)
{
    for(auto reroute:info)
    {
        int netId = reroute.netgrids->NetId;
        graph->updateNetGrids(netId,reroute.netgrids);
        graph->updateTree(netId,reroute.nettree);
    }
}

void OnlyRouting(Graph*graph,std::string fileName,const std::vector<std::string> &cellinfo)
{

    int BestSc = graph->score;
    int success = 0;
    time_t t1,t2,t3;
    t1 = time(NULL);
    t2 = time(NULL);
    int interval = 60;
    for(int i = graph->Nets.size();i>=1;i--)
    {
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
        TwoPinNets twopins;
        get_two_pins(twopins,graph->getNet(i));
        std::pair<ReroutInfo,bool> result = Reroute(graph,net->NetId,twopins);
        
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
            // if(graph->score < BestSc)//改成SA
            if(true)
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
                    std::cout<<"Net : "<<i<<"\n";
                    OutPut(graph,fileName,cellinfo);
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
    std::cout<<"score = "<<graph->score<<"\n";
    OutPut(graph,fileName,cellinfo);
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

void RoutingWithCellMOV(Graph*graph,std::string fileName,std::vector<std::string>&movingCellInfo,bool Ripall)
{
    graph->placementInit();
    int mov = 0;
    int success = 0;
    std::pair<std::string,CellInst*>movcellPair;
    float BestSc = graph->score;
    time_t t1,t2,t3;
    t1 = time(NULL);
    t2 = time(NULL);
    int interval = 60;
    while( (movcellPair = graph->cellMoving()).second )//
    {   
        CellInst* movCell = movcellPair.second;
        mov++;
        // if(mov>graph->MAX_Cell_MOVE/20)
        // {
        //     movCell->row = movCell->originalRow;
        //     movCell->col = movCell->originalCol;
        //     graph->insertCellsBlkg(movCell);
        //     break;
        // }
        
        movingCellInfo.push_back(movcellPair.first+" "+std::to_string(movCell->row)+" "+std::to_string(movCell->col));
        
        if(movingCellInfo.size()>graph->MAX_Cell_MOVE)
        {
            movingCellInfo.pop_back();
            movCell->row = movCell->originalRow;
            movCell->col = movCell->originalCol;
            graph->insertCellsBlkg(movCell);
            break;
        }

        //-----------------------------------------------RelatedNets------------------------------------------------------
        std::map<Net*,int>Nets = RelatedNets(movCell);

        //-------------------------------------------------Routing Init---------------------------------------------------
        bool movingsuccess = true;
        std::vector<ReroutInfo>infos;infos.reserve(Nets.size());//each time a routing success , insert nettree/netgrids.
        std::vector<int>RipId;RipId.reserve(Nets.size());//each time rip-up , need insert netId to RipId.
        //-------------------------------------------------Routing Init---------------------------------------------------

        
        //---------------------------------------------------RipUP----------------------------------------------------------
        if(Ripall){
            for(auto net:Nets)//Ripup all related
            {
                int netId = std::stoi(net.first->netName.substr(1,-1));
                RipUpNet(graph,graph->getNetGrids(netId));RipId.push_back(netId);
                
            }
        }

        for(auto net:Nets)
        {
            int netId = std::stoi(net.first->netName.substr(1,-1));

            if(!Ripall){
                RipUpNet(graph,graph->getNetGrids(netId));RipId.push_back(netId);
            }
            //---------------------------------------------------RipUP----------------------------------------------------------

            //-------------------------------------------------Routing----------------------------------------------------------
            TwoPinNets twopins;
            get_two_pins(twopins,*net.first);
            std::pair<ReroutInfo,bool> result = Reroute(graph,netId,twopins);

            if(result.second==false)//failed
            {
                movingsuccess = false;
                break;
            }
            else{//success
                AddingNet(graph,result.first.netgrids);
                infos.push_back(result.first);
            }
            //-------------------------------------------------Routing----------------------------------------------------------
        }
            

        //-------------------------------------------------Accept or Reject-------------------------------------------------
        if(movingsuccess)
        {
            if(graph->score < BestSc)  //Accept
            {
                Accept(graph,infos);
                BestSc = graph->score;
                success++;
                t3 = time(NULL);
                if(t3 > t2 + interval)
                {
                    t2 = time(NULL);
                    std::cout<<"spend "<<t3-t1<<" seconds\n";
                    std::cout<<"Best = "<<BestSc<<"\n";
                    OutPut(graph,fileName,movingCellInfo);
                }
            }
            else{                     //Reject
                movingCellInfo.pop_back();
                Reject(graph,infos,RipId);
                movCell->row = movCell->originalRow;
                movCell->col = movCell->originalCol;
                graph->insertCellsBlkg(movCell);
            }
        }
        else{                        //Reject
            movingCellInfo.pop_back();
            Reject(graph,infos,RipId);
            movCell->row = movCell->originalRow;
            movCell->col = movCell->originalCol;
            graph->insertCellsBlkg(movCell);
        }
        //-------------------------------------------------Accept or Reject-------------------------------------------------
    }
    std::cout<<"count = "<<mov<<"\n";
    std::cout<<"sucess = "<<success<<"\n";
    OutPut(graph,fileName,movingCellInfo);
    std::cout<<"score = "<<graph->score<<"\n";
    t2 = time(NULL);
    std::cout<<"spend "<<t2-t1<<" seconds\n";
   
}

void OutPut(Graph*graph,std::string fileName,const std::vector<std::string>&MovingCell)
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

