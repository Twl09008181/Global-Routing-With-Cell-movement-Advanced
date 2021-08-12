#include "header/graph.hpp"
#include "header/Routing.hpp"
#include "header/TwoPinNet.hpp"
int LAYER_SEARCH_RANGE =3;
float ESCAPE_W = 10;
float VIA_W = 1;


Graph* graph = nullptr;

int show_demand(Graph*graph,bool show = true);
void Init( std::string path,std::string fileName)
{
    graph = new Graph(path+fileName);
}

void RoutingSchedule(Graph*graph);

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

    // std::cout<<"graph Init done!\n";
    // std::cout<<"Inital routing demand:\n";
    //show_demand(graph);



    //Expand testing
    // for(int i = 1;i<=graph->Nets.size();i++)
    // {
    //     int OriginDmd = show_demand(graph,false); 
    //     RipUpNet(graph,&graph->getNet(i),graph->getTree(i));
    //     //show_demand(graph);
    //     ExpandTree(graph->getTree(i));
    //     AddingNet(graph,&graph->getNet(i),graph->getTree(i));
    //     int ExpandDmd = show_demand(graph,false);
    //     if(ExpandDmd!=OriginDmd)
    //     {
    //         std::cerr<<"error : Dmd dosen't match!\n";
    //         exit(1);
    //     }
    //     std::cout<<i<<"\n";
    // }
    // std::cout<<"check done!\n";


    //RoutingSchedule(graph);


    // auto t = graph->getTree(1);
    // for(auto endpoint:t->EndPoint)
    // {
    //     std::cout<<endpoint->row<<" "<<endpoint->col<<" "<<endpoint->lay<<"\n";
    // }
    delete graph;
	return 0;
}


void LabelIntree(node*v)
{
    do
    {
        v->IsIntree = true;
        v = v->parent;
    }while(v->parent&&!v->parent->IsIntree);
}


bool accept;
//routing interface必須有 繞線失敗就 recover demand 的功能
tree* Tree2Tree(Graph*graph,Net*net,tree*t1,tree*t2)
{
    if(t1==t2)return t1;//pre check


    //bool accept = false;//設定false來測試Rip up機制

    //bool accept = true;

    
    //routing procedure---------------------------------------- 還沒完成 差這裡而已了

    //Step 1 Target all Ggrid of tree2
    TargetNet(graph,net,t2);//target 

    //Step2 Expand all Ggrid of tree1 
    //ExpandTree(t1);//是不是每次都需要Expand 再討論,或者一開始expand就好了

    //Step 3 dijkstra   (還要設計cost function canroute等function)
    std::priority_queue<node*>Q;


    //這邊用unordermap做search 存posstr,cost (存grid的前一個cost)
    std::unordered_map<std::string,float>gridCost;

    for(auto n:t1->all){//全部丟入Q    
        n->cost = 0;//setting cost = 0
        Q.push(n);//using n->cost
        gridCost[pos2str(n->p)] = 0;
    }

    //Step 3 繞出來則設定sucess = true  然後  routing只存segment point (要backtrace 回去)
    
    //"每次都要新增新的node而不是原本的,就算是目標也一樣"
    //概念如下:
    //使用"找到t2的那個點"(不是t2) 做backtrack直到 到了一個inTree的點 這段全部tag為intree
    //然後新增一個跟t2的那個點一樣的node,把他加入tree,然後用那個點對她做connect
    //然後進行clear not inTree的動作

    //if failed, set success to false 
    
    //else 
    //LabelIntree(v); v是新增的t2的那個點

    //clear not Intree
    for(auto n:t1->all)//要確認一下是否能在迴圈中移除node,vector一定不行,linked list不一定
    {
        if(!n->IsIntree){
            t1->all.remove(n);
            t1->leaf.erase(n);
            //還要記的delete掉
        }
    }
    //進行 Shrink
    //Shrink(t1); //不需要,要用的話每次都要expand



    
    //future : 
    //1. bounding box
    //2. multicore of two-pin-nets


    //routing procedure----------------------------------------


    //繞線完成後要將t2當中的leaf,all加入倒t1當中 並回傳t1 ,如果失敗就回傳nullptr
    if(accept){
        if(t1!=t2){
            for(auto l:t2->leaf){
                t1->leaf.insert(l);
                l->routing_tree = t1;
            }
            for(auto n:t2->all){
                t1->all.push_back(n);
                n->routing_tree = t1;
            }
            t2->leaf.clear();
            t2->all.clear();
        }
        return t1;
    }
    else {
        return nullptr;
    }
}
std::pair<tree*,bool> Reroute(Graph*graph,Net*net,TwoPinNets&twopins)
{
    int initdemand = TwoPinNetsInit(graph,net,twopins);//Init
    tree*T= nullptr;
    for(auto pins:twopins)
    {
        T = Tree2Tree(graph,net,pins.first->routing_tree,pins.second->routing_tree);
        if(!T) //把整個two-pin nets 繞線產生出來的tree全部collect成一棵回傳
        {
            std::set<tree*>collect;//set(避免duplicate delete)
            for(auto pins:twopins)
            {
                collect.insert(pins.first->routing_tree);
                collect.insert(pins.second->routing_tree);
            }
            tree* CollectTree = new tree;
            for(tree* t:collect)
            {
                for(node * pin : t->leaf){CollectTree->leaf.insert(pin);}//for rip-up
                for(node * pin : t->all){CollectTree->all.push_front(pin);}//for delete
            }
            return {CollectTree,false};
        }
    }
    return {T,true};
}

// void RoutingSchedule(Graph*graph)
// {
//     graph->placementInit();

//     CellInst* movCell;
//     int mov = 0;
//     int success = 0;
//     while( (movCell = graph->cellMoving()))//到時候可改多個Cell移動後再reroute,就把net用set收集後再一起reroute
//     {
//         mov++;
//         bool movingsuccess = true;
//         std::vector<tree*>netTrees(movCell->nets.size(),nullptr);

//         //Reroute all net related to this Cell
//         int lastFaileIdx = 0;
//         std::cout<<"------Starting-------\n";
//         show_demand(graph);
//         std::cout<<"accept or not?\n";
//         std::cin>>accept;

//         for(int i = 0;i<movCell->nets.size();i++)
//         {
//             auto net = movCell->nets.at(i); 
//             RipUpNet(graph,net);
//             TwoPinNets twopins;
//             get_two_pins(twopins,*net);
//             auto result = Reroute(graph,net,twopins);
//             netTrees.at(i) = result.first;

//             if(result.second==false)
//             {
//                 lastFaileIdx = i;
//                 movingsuccess = false;
//                 break;
//             }
//         }
//         if(movingsuccess)//replace old tree
//         {
//             std::cout<<"updating!\n";
//             for(int i = 0;i<movCell->nets.size();i++)
//             {
//                 int NetId = std::stoi(movCell->nets.at(i)->netName.substr(1,-1));
//                 graph->updateTree(NetId,netTrees.at(i));
//             }
//             success++;
//         }
//         else{///failed recover old tree demand
            
//             std::cout<<"Before ripup\n";
//             show_demand(graph);
//             //RipUp 這次繞線
//             for(int i = 0;i<=lastFaileIdx;i++)
//             {
//                 auto &net = movCell->nets.at(i);
//                 RipUpNet(graph,net,netTrees.at(i));//recover demand
//             }
            
//             std::cout<<"After ripup\n";
//             show_demand(graph);

//             for(int i = 0;i<=lastFaileIdx;i++){delete netTrees.at(i);}//delete 

//             //重新復原demand
//             for(int i = 0;i<=lastFaileIdx;i++)
//             {
//                 auto &net = movCell->nets.at(i);
//                 AddingNet(graph,net);//recover demand
//             }
//         }
//         std::cout<<"After routing!\n";
//         show_demand(graph);
//     }
//     std::cout<<"final demand:\n";
//     show_demand(graph);
//     std::cout<<"count = "<<mov<<"\n";
//     std::cout<<"sucess = "<<success<<"\n";
// }



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
