#include "header/graph.hpp"
#include "header/Routing.hpp"
#include "header/TwoPinNet.hpp"

Graph* graph = nullptr;

int show_demand(Graph*graph,bool show = true);
void Init( std::string path,std::string fileName)
{
    graph = new Graph(path+fileName);
}

void RoutingSchedule(Graph*graph);
std::pair<tree*,bool> Reroute(Graph*graph,Net*net,TwoPinNets&twopins);
void OnlyRouting(Graph*graph,std::string fileName);
void OutPut(Graph*graph,std::string fileName);

class minCost{
public:
    minCost() = default;
    bool operator()(const node* n1,const node*n2){return n1->cost > n2->cost;}
};

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
    // std::cout<<"Inital routing demand:\n";
    show_demand(graph);

    
    OnlyRouting(graph,fileName);
    //RoutingSchedule(graph);


    // PrintAll(graph);

 
    delete graph;
	return 0;
}


void OnlyRouting(Graph*graph,std::string fileName)
{

    int bestDmd = show_demand(graph);
    for(int i = 1;i<=graph->Nets.size();i++)
    {
        auto &net = graph->getNet(i);
        // std::cout<<"Pins:\n";
        // for(auto pin:net.net_pins)
        // {
        //     std::cout<<pin.first->row<<" "<<pin.first->col<<" "<<(*pin.first->mCell).pins[pin.second]<<"\n";
        // }

        RipUpNet(graph,&net);
        TwoPinNets twopins;
        get_two_pins(twopins,net);
        auto result = Reroute(graph,&net,twopins);
        tree * netTree = result.first;
        if(result.second==false)
        {
           net.routingState = Net::state::CanAdd;
           //RipUpNet(graph,&net,netTree);
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
    OutPut(graph,fileName);
    //printTree(graph->getTree(1),graph->getNet(1).netName);
  
}
void OutPut(Graph*graph,std::string fileName)
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
    os<<"NumMovedCellInst 0\n";
    os<<"NumRoutes "<<NumRoutes<<"\n";

    for(auto s:segments)
    {
        os<<s<<"\n";
    }
    std::cout<<"saving done!\n";
}


bool IsIntree(Graph*graph,Net*net,node*v)
{
    auto &grid = (*graph)(v->p.row,v->p.col,v->p.lay);
    return grid.enrollNet == net;
}
void LabelIntree(Graph*graph,Net*net,node*v,std::unordered_map<std::string,bool>&t1Point)
{
    //std::cout<<"find label!\n";
    while(v)
    {
        //std::cout<<v->p<<"\n";
        auto &grid = (*graph)(v->p.row,v->p.col,v->p.lay);
        if(t1Point.find(pos2str(v->p))!=t1Point.end())break;
        Enroll(grid,net);
        v = v->parent;
    }
}




node* Search(Graph*graph,Net*net,node *v,const pos&delta,std::unordered_map<std::string,float>&gridCost)
{
    //Dir checking
    if(delta.row!=0&&v->p.lay%2==1){return nullptr;}//error routing dir
    if(delta.col!=0&&v->p.lay%2==0){return nullptr;}//error routing dir

    //Boundary checking
    pos P = {v->p.row+delta.row,v->p.col+delta.col,v->p.lay+delta.lay};
    if(P.row<graph->RowBound().first||P.row>graph->RowBound().second){return nullptr;} //RowBound checking
    if(P.col<graph->ColBound().first||P.col>graph->ColBound().second){return nullptr;} //ColBound checking
    if(P.lay<net->minLayer||P.lay>graph->LayerNum()){return nullptr;}//minLayer checking
    
    //Capacity checking
    Ggrid& g = (*graph)(P.row,P.col,P.lay);
    bool enough = (g.enrollNet==net)? true : g.get_remaining(); 
    if(!enough)return nullptr;


    //Caculate Cost    
    std::string str = pos2str(P);
    float lastCost = (gridCost.find(str)==gridCost.end())? FLT_MAX:gridCost[str];
    float pf = graph->getLay(P.lay).powerFactor;
    float weight = net->weight;
    
    node * n = new node(P);
    n->cost = (g.enrollNet==net)? 0:weight*pf+v->cost; //enroll只有在確定繞線完成時才加入

    if(n->cost<lastCost)
    {
        v->routing_tree->addNode(n);
        v->connect(n);
        gridCost[str] = n->cost;
    }
    else{
        delete n;
        n = nullptr;
    }
    return n;
}

bool AssingPesudo(Graph*graph,Net*net,node*n)
{
    //std::cout<<"Assign pesudo!\n";
    float Bestcost = FLT_MAX;
    int bestLay = net->minLayer;
    for(int i = net->minLayer;i<=graph->LayerNum();i++)
    {
        auto grid = (*graph)(n->p.row,n->p.col,i);
        int netcost = 0;
        if(grid.enrollNet==net)
        {
            n->p.lay = i;
            return true;
        }
        else 
            netcost = 1/(1+exp2(grid.get_remaining()));
        if(netcost<Bestcost)
        {
            Bestcost = netcost;
            n->p.lay = i;
        }
    }

    if(n->p.lay==-1){return false;}
    if((*graph)(n->p.row,n->p.col,n->p.lay).get_remaining()==0){return false;}

    (*graph)(n->p.row,n->p.col,n->p.lay).enrollNet = net;
    return true;
}


   //future : 
    //1. bounding box
    //2. multicore of two-pin-nets
tree* Tree2Tree(Graph*graph,Net*net,tree*t1,tree*t2)
{
    if(t1==t2)return t1;//precheck

    //std::cout<<"routing t1\n";
    // for(auto n:t1->all)
    // {
    //     std::cout<<n->p<<"\n";
    // }
    // std::cout<<"routing t2\n";
    //   for(auto n:t2->all)
    // {
    //     std::cout<<n->p<<"\n";
    // }

    //std::cout<<"Step1\n";
    //Step 1 Setting Target&Source 
    TargetTree(graph,net,t2);
    SourceTree(graph,net,t1);
 

    //std::cout<<"Step2\n";
    //Step 2 Prepare Q
    std::priority_queue<node*,std::vector<node*>,minCost>Q;
    std::unordered_map<std::string,float>gridCost;//再想想要放哪
    std::unordered_map<std::string,bool>t1Point;//用來backtrack
    std::unordered_map<std::string,node*>t2Pesudo;//用來update two-pin-net sets...

    //Multi Source
    bool InitSource = true;
    for(auto n:t1->all){//全部丟入Q    //至此12,27 1仍為leaf
        n->cost = 0;
        Q.push(n);
        gridCost[pos2str(n->p)] = 0;
        t1Point[pos2str(n->p)] = true;
    }
    bool t2IsPesudo = false;
    for(auto n:t2->all){
        if(n->p.lay==-1){ 
            t2IsPesudo = true;
            std::string twoDpos = std::to_string(n->p.row)+" "+std::to_string(n->p.col);
            t2Pesudo[twoDpos] = n;
        }  
    }


    //std::cout<<"Step3\n";
    //Step 3 Search
    node *targetPoint = nullptr;
    while(!Q.empty()&&!targetPoint&&InitSource)
    {
        node * v = Q.top();Q.pop();
        if((*graph)(v->p.row,v->p.col,v->p.lay).isTarget)//find
        {
            std::string twoDpos = std::to_string(v->p.row)+" "+std::to_string(v->p.col);
            targetPoint = v;
            if(t2Pesudo.find(twoDpos)!=t2Pesudo.end())
            {
                t2Pesudo[twoDpos]->p = v->p;
            }
            break;
        }
        node * u = nullptr;
        if(u = Search(graph,net,v,{0,0,1},gridCost)) //up
            Q.push(u);
        if(u = Search(graph,net,v,{0,0,-1},gridCost))//down
            Q.push(u);
        if(u = Search(graph,net,v,{0,1,0},gridCost))//-col
            Q.push(u);
        if(u = Search(graph,net,v,{0,-1,0},gridCost))//+col
            Q.push(u);
        if(u = Search(graph,net,v,{1,0,0},gridCost))//+row
            Q.push(u);
        if(u = Search(graph,net,v,{-1,0,0},gridCost))//-row
            Q.push(u);
        // std::cout<<v->p<<"\n";
    }

    //Untarget
    UntargetTree(graph,net,t2);

    if(t2IsPesudo)
    {
        tree *  pesudoClear = new tree;
        for(auto pesudoPin:t2Pesudo)
        {
            node *pesudoNode = new node(pesudoPin.second->p); 
            pesudoNode->p.lay = -1;
            pesudoClear->addNode(pesudoNode);
        }
        UntargetTree(graph,net,pesudoClear);
        delete pesudoClear;
    }


    //std::cout<<"Step4\n";
    //Step 4
    if(targetPoint){//and still need update the t2....
        //std::cout<<"success!\n";
        LabelIntree(graph,net,targetPoint,t1Point);
    }

    //std::cout<<"Step5\n";
    //clear not Intree
    std::list<node*>recycle;
    for(auto n:t1->all)//要確認一下是否能在迴圈中移除node,vector一定不行,linked list不一定
    {
        if(!IsIntree(graph,net,n)){
            recycle.push_front(n);
            t1->leaf.erase(n);
            t1->leaf.insert(n->parent);  //Bug Fix!!!
        }
    }
    for(auto n:recycle){
        t1->all.remove(n);
        t1->leaf.erase(n);
    }

    //std::cout<<"Step6\n";
    if(targetPoint){
        if(t1!=t2){
            //std::cout<<"combine!\n";
            for(auto l:t2->leaf){
                t1->leaf.insert(l);
                l->routing_tree = t1;
            }
            for(auto n:t2->all){
                t1->all.push_back(n);
                n->routing_tree = t1;
                //std::cout<<n->p<<"\n";
            }
            //std::cout<<"combine done!\n";
            t2->leaf.clear();
            t2->all.clear();
        }
        return t1;//繞線完成後要將t2當中的leaf,all加入倒t1當中 並回傳t1 ,如果失敗就回傳nullptr
    }
    else {
        return nullptr;
    }
}
std::pair<tree*,bool> Reroute(Graph*graph,Net*net,TwoPinNets&twopins)
{
    //std::cout<<"init"<<"\n";
    int initdemand = TwoPinNetsInit(graph,net,twopins);//Init
    //std::cout<<initdemand<<"\n";

    tree*T= nullptr;
    //std::cout<<"TwopinNet!\n";
    for(auto pins:twopins)
    {   
        if(initdemand!=-1)
            T = Tree2Tree(graph,net,pins.first->routing_tree,pins.second->routing_tree);
        
        if(!T) //把整個two-pin nets 繞線產生出來的tree全部collect成一棵回傳
        {
            //std::cout<<"recycle!\n";
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
            TreeInterface(graph,net,EnrollNocheck,CollectTree);
            net->routingState = Net::state::Routing;//
            UnRegisterTree(graph,net,CollectTree);
            net->routingState = Net::state::doneAdd;//
            return {CollectTree,false};
        }
    }

    UnRegisterTree(graph,net,T);
    AddingNet(graph,net,T);
    return {T,true};
}




//現在先不用回傳最佳,有答案就Ok
void RoutingSchedule(Graph*graph)
{
    graph->placementInit();

    
    int mov = 0;
    int success = 0;
    std::pair<std::string,CellInst*>movcellPair;
    while( (movcellPair = graph->cellMoving()).second )//到時候可改多個Cell移動後再reroute,就把net用set收集後再一起reroute
    {   
        CellInst* movCell = movcellPair.second;
        mov++;
        bool movingsuccess = true;
        std::vector<tree*>netTrees(movCell->nets.size(),nullptr);

        //Reroute all net related to this Cell
        int LastSuccess = 0;
        //std::cout<<"------Starting-------\n";
        std::cout<<movcellPair.first<<" "<<movCell->row<<" "<<movCell->col<<"\n";

        for(int i = 0;i<movCell->nets.size();i++)//scan net
        {
            //std::cout<<i<<"\n";
            auto net = movCell->nets.at(i); 
            RipUpNet(graph,net);
            TwoPinNets twopins;
            get_two_pins(twopins,*net);
            auto result = Reroute(graph,net,twopins);
            netTrees.at(i) = result.first;
            if(result.second==false)
            {
                LastSuccess = i-1;
                movingsuccess = false;
                break;
            }
        }
        if(movingsuccess)//replace old tree
        {
            std::cout<<"success!\n";
            //std::cout<<"updating!\n";
            for(int i = 0;i<movCell->nets.size();i++)
            {
                int NetId = std::stoi(movCell->nets.at(i)->netName.substr(1,-1));
                graph->updateTree(NetId,netTrees.at(i));
            }
            success++;
        }
        else{///failed recover old tree demand
            
            //std::cout<<"Before ripup\n";
            //show_demand(graph);
            //RipUp 這次繞線
            for(int i = 0;i<=LastSuccess;i++)//這邊到時候也有問題 因為要設計成成功才加demand
            {
                auto &net = movCell->nets.at(i);
                RipUpNet(graph,net,netTrees.at(i));//recover demand
            }
            
            //std::cout<<"After ripup\n";
            //show_demand(graph);

            for(int i = 0;i<=LastSuccess;i++){delete netTrees.at(i);}//delete 

            //重新復原demand
            for(int i = 0;i<=LastSuccess;i++)
            {
                auto &net = movCell->nets.at(i);
                AddingNet(graph,net);//recover demand
            }

            //復位
            movCell->row = movCell->originalRow;
            movCell->col = movCell->originalCol;
        }
        std::cout<<"After routing!\n";
        show_demand(graph);
    }
    std::cout<<"final demand:\n";
    show_demand(graph);
    PrintAll(graph);
    std::cout<<"count = "<<mov<<"\n";
    std::cout<<"sucess = "<<success<<"\n";
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
