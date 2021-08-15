#include "header/graph.hpp"
#include "header/Routing.hpp"
#include "header/TwoPinNet.hpp"

Graph* graph = nullptr;

int show_demand(Graph*graph,bool show = true);
void Init( std::string path,std::string fileName){graph = new Graph(path+fileName);}

void OnlyRouting(Graph*graph,std::string fileName);
void OutPut(Graph*graph,std::string fileName,std::vector<std::string>MovingCell);
void RoutingWithCellMOV(Graph*graph,std::string fileName);

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
    std::cout<<"Inital routing demand:\n";
    show_demand(graph);

    
    OnlyRouting(graph,fileName);
    //RoutingWithCellMOV(graph,fileName);


    

    // float avgTreeNum = 0;

    // for(int i = 1;i<=graph->Nets.size();i++){
    //     auto &net= graph->getNet(i);
    //     TwoPinNets twopin;
    //     get_two_pins(twopin,net);

    //     std::set<tree*>TreeRecord;
    //     for(auto pins:twopin)
    //     {
    //         //std::cout<<pins.first->p<<" "<<pins.second->p<<"\n";
    //         TreeRecord.insert(pins.first->routing_tree);
    //         TreeRecord.insert(pins.second->routing_tree);
    //     }
    //     avgTreeNum+=TreeRecord.size();
    // }
    // std::cout<<avgTreeNum/graph->Nets.size()<<"\n";
 
    delete graph;
	return 0;
}



void OnlyRouting(Graph*graph,std::string fileName)
{

    int bestDmd = show_demand(graph);
    for(int i = 1;i<=graph->Nets.size();i++)
    {
        auto &net = graph->getNet(i);
        RipUpNet(graph,&net);
        TwoPinNets twopins;
        get_two_pins(twopins,net);
        auto result = Reroute(graph,&net,twopins);
        tree * netTree = result.first;
        if(result.second==false)
        {
           net.routingState = Net::state::CanAdd;
           delete netTree;
           AddingNet(graph,&net);//recover demand
        }
        else{
            graph->updateTree(i,netTree);
        }
        std::cout<<"After routing!\n";
        int dmd = show_demand(graph);
        if(dmd<bestDmd)
        {
            bestDmd = dmd;
            //OutPut(graph,fileName);
        }
    }
    OutPut(graph,fileName,{});
}

void RoutingWithCellMOV(Graph*graph,std::string fileName)
{
    graph->placementInit();
    std::vector<std::string>movingCellInfo;
    int mov = 0;
    int success = 0;
    std::pair<std::string,CellInst*>movcellPair;
    while( (movcellPair = graph->cellMoving()).second )//
    {   
        mov++;
        CellInst* movCell = movcellPair.second;
        bool movingsuccess = true;
        
        movingCellInfo.push_back(movcellPair.first+" "+std::to_string(movCell->row)+" "+std::to_string(movCell->col));
        
        std::map<Net*,int>Nets;//有一些net重複了

        int Idx = 0;
        for(auto net:movCell->nets){
            if(Nets.find(net)==Nets.end())
                Nets.insert({net,Idx++});
        }
        std::vector<tree*>netTrees(Nets.size(),nullptr);

        auto Last = Nets.begin();
        for(;Last!=Nets.end();++Last){
            
            
            auto &net = Last->first;
            //std::cout<<net->netName<<"\n";
            int netTreeIdx = Last->second;
            RipUpNet(graph,net);
            TwoPinNets twopins;
            get_two_pins(twopins,*net);
            auto result = Reroute(graph,net,twopins);
            netTrees.at(netTreeIdx) = result.first;
            if(result.second==false)
            {
                movingsuccess = false;
                break;
            }
        }
        if(movingsuccess)//replace old tree
        {
            for(auto netInfo:Nets)
            {
                auto net = netInfo.first;
                int NetId = std::stoi(net->netName.substr(1,-1));
                graph->updateTree(NetId,netTrees.at(netInfo.second));
            }
            success++;
        }

        else{
            movingCellInfo.pop_back();
            for(auto ptr = Nets.begin();ptr!=Last;ptr++)
            {
                auto net = ptr->first;
                RipUpNet(graph,net,netTrees.at(ptr->second));
            }
            Last++;
            for(auto ptr = Nets.begin();ptr!=Last;ptr++){delete netTrees.at(ptr->second);}//delete 
            for(auto ptr = Nets.begin();ptr!=Last;ptr++)
            {
                auto &net = ptr->first;
                net->routingState = Net::state::CanAdd;
                AddingNet(graph,net);//recover demand
            }
            movCell->row = movCell->originalRow;
            movCell->col = movCell->originalCol;
            graph->insertCellsBlkg(movCell);
        }

        std::cout<<"After routing!\n";
        show_demand(graph);
    }
    std::cout<<"count = "<<mov<<"\n";
    std::cout<<"sucess = "<<success<<"\n";
    OutPut(graph,fileName,movingCellInfo);
}



int show_demand(Graph*graph,bool show)
{
    int LayerNum = graph->LayerNum();
    std::pair<int,int>Row = graph->RowBound();
    std::pair<int,int>Col = graph->ColBound();

    int total_demand = 0;
    for(int lay = 1;lay <= LayerNum;lay++)
    {
        //#ifdef PARSER_TEST
        //std::cout<<"Layer : "<< lay <<"\n";
        //#endif
        for(int row = Row.second;row >=Row.first ; row--)
        {
            for(int col = Col.first; col <= Col.second; col++)
            {
                // #ifdef PARSER_TEST
                //std::cout<<graph(row,col,lay).demand<<" ";
                //#endif
                total_demand+=(*graph)(row,col,lay).demand;
            }
            //#ifdef PARSER_TEST
            //std::cout<<"\n";
            //#endif
        }
    }
    if(show)
    std::cout<<"Total :"<<total_demand<<"\n";
    return total_demand;
}
void OutPut(Graph*graph,std::string fileName,std::vector<std::string>MovingCell)
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
}

