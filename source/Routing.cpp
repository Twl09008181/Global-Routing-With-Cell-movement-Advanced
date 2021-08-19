#include "../header/Routing.hpp"


bool node::IsSingle()//Is leaf and no parent
{
    return routing_tree->leaf.find(this)!=routing_tree->leaf.end()&&!parent;
}
void node::connect(node *host)
{
    if(this->parent!=host&&this!=host)
    {   
        child.insert(host);
        host->parent = this;
        this->routing_tree->leaf.erase(this);
    }

    //future:adding host to routing tree (Broadcast)
}

//-------------------------------------------------node Member function-----------------------------------------------------------




//-------------------------------------------------callback functions------------------------------------------------------------


bool passing(Ggrid*grid,NetGrids*net)
{
    if(net->AlreadyPass(grid))
        return true;
    else{
        if(grid->get_remaining()){
            net->PassGrid(grid);
            return true;
        }
    }
    return false;
}

bool target(Ggrid*g,NetGrids*net){
    g->isTarget = true;    
    return true;
}
bool Untarget(Ggrid*g,NetGrids*net)
{
    g->isTarget = false;
    return true;
}
//-------------------------------------------------callback functions------------------------------------------------------------




//--------------------------------------------------two-pin-sets function---------------------------------------------------
int TwoPinNetsInit(Graph*graph,NetGrids*net,TwoPinNets&pinset)
{
    int totalInit = 0;
    bool canInit = false;
    for(auto &twopin:pinset)
    {
        pos pin1 = twopin.first->p;
        pos pin2 = twopin.second->p;
        //std::cout<<"pin1!\n";
        if(pin1.lay!=-1){  (canInit = passing(&(*graph)(pin1.row,pin1.col,pin1.lay),net)==true) ? totalInit+=1:totalInit+=0;} //need
        //std::cout<<"canInit ="<<canInit<< "!\n";
        if(pin2.lay!=-1&&canInit){(canInit = passing(&(*graph)(pin2.row,pin2.col,pin2.lay),net)==true) ? totalInit+=1:totalInit+=0;}//need
        //std::cout<<"canInit ="<<canInit<< "!\n";
        if(!canInit){
            // std::cout<<"return -1!\n";
            return -1;
        }
    }
    return totalInit;
}

void Sgmt_Init(node*v,node*u,pos &PosS,pos&PosT,pos &PosDelta)
{
    int sRow = v->p.row;
    int sCol = v->p.col;
    int sLay = v->p.lay;

    int tRow = u->p.row;
    int tCol = u->p.col;
    int tLay = u->p.lay;
            
    int d_r = (sRow==tRow)? 0 : ( (sRow>tRow)? -1:1);
    int d_c = (sCol==tCol)? 0 : ( (sCol>tCol)? -1:1);
    int d_l = (sLay==tLay)? 0 : ( (sLay>tLay)? -1:1);  
    using std::abs;
    int check = abs(d_r) + abs(d_c) + abs(d_l);
    if(check>1){std::cerr<<"error in Sgmt_Init!!! Input: "<<u->p<<" " << v->p <<" is not a segment\n";exit(1);}
    
    PosS = pos{sRow,sCol,sLay};
    PosT = pos{tRow,tCol,tLay};
    PosDelta = pos{d_r,d_c,d_l};
}

void Sgmt_Grid(Graph*graph,NetGrids*net,node*v,node*u,bool(*f)(Ggrid*,NetGrids*))
{
    pos PosS,PosT,PosDelta;
    Sgmt_Init(v,u,PosS,PosT,PosDelta);
    do{
        auto &grid = (*graph)(PosS.row,PosS.col,PosS.lay);
        f(&grid,net);
        PosS.row+=PosDelta.row;
        PosS.col+=PosDelta.col;
        PosS.lay+=PosDelta.lay;
    }while(PosS!=PosT);
    auto &grid = (*graph)(PosS.row,PosS.col,PosS.lay);
    f(&grid,net);//last
}


void Backtack_Sgmt_Grid(Graph*graph,NetGrids*net,node*v,bool(*f)(Ggrid*,NetGrids*))
{
    while(v->parent)
    {
        Sgmt_Grid(graph,net,v,v->parent,f);
        v = v->parent;
    }
}
//------------------------------------------------Demand Interface------------------------------------------------------------------------


void RipUpNet(Graph*graph,NetGrids*net)
{
    for(auto &grid:net->grids)
    {
        if(grid.second==true)
        {
            //std::cout<<"delete!\n";
            grid.first->delete_demand();
            grid.second = false;
        }
        
    }
}
void AddingNet(Graph*graph,NetGrids*net)
{
    for(auto &grid:net->grids)
    {
        net->Add(grid.first);
    }
}

void RipUpAll(Graph*graph)
{
    
    for(int i = 1;i<=graph->Nets.size();i++){
        NetGrids* net = graph->getNetGrids(i); 
        RipUpNet(graph,net);
    }
}
void AddingAll(Graph*graph)
{
    for(int i = 1;i<=graph->Nets.size();i++){
        NetGrids* net = graph->getNetGrids(i);   
        AddingNet(graph,net);
    }
}
class minCost{
public:
    minCost() = default;
    bool operator()(const node* n1,const node*n2){return n1->cost > n2->cost;}
};
void TreeInterface(Graph*graph,NetGrids*net,bool(*callback)(Ggrid* ,NetGrids*),tree* nettree)
{
    tree *t = nullptr;
    if(nettree){
        t = nettree;
    }
    else
        t = graph->getTree(net->NetId);

    for(auto leaf:t->leaf){
        if(leaf->IsSingle()&&leaf->p.lay!=-1)
            callback(&(*graph)(leaf->p.row,leaf->p.col,leaf->p.lay),net);
        else if(leaf->p.lay!=-1)
            Backtack_Sgmt_Grid(graph,net,leaf,callback);
        else if(leaf->p.lay==-1&&callback==target)//important
        {
            int minLay = graph->getNet(net->NetId).minLayer;
            for(int i = minLay;i<=graph->LayerNum();i++)//label all layer be target
                (*graph)(leaf->p.row,leaf->p.col,i).isTarget = true;  
        }
        else if(leaf->p.lay==-1&&callback==Untarget)
        {
            int minLay = graph->getNet(net->NetId).minLayer;
            for(int i = minLay;i<=graph->LayerNum();i++)//label all layer be target
                (*graph)(leaf->p.row,leaf->p.col,i).isTarget = false;  
        }
    }
}



//Output interface---------------------
void backTrackPrint(Graph*graph,Net*net,node*v,std::vector<std::string>*segment)
{
    while (v->parent)
    {
        auto &g = (*graph)(v->p.row,v->p.col,v->p.lay);
        if(g.enrollNet!=net){
            std::cout<<"!!break\n";
            break;
        }
        std::string posv = pos2str(v->p);
        std::string posu = pos2str(v->parent->p);
        if(segment){segment->push_back(posu+" "+posv+" "+net->netName);}
        else{std::cout<<(posu+" "+posv+" "+net->netName)<<"\n";}
        v = v->parent;
    }
    
}

void printTree(Graph*graph,Net*net,tree*t,std::vector<std::string>*segment)
{
    for(auto leaf:t->leaf){
        if(!leaf->IsSingle()&&leaf->p.lay!=-1)
            backTrackPrint(graph,net,leaf,segment);
    }
}
void PrintAll(Graph*graph,std::vector<std::string>*segment)
{
    for(int i = 1;i<=graph->Nets.size();i++){
        auto t = graph->getTree(i);
        auto &net = graph->getNet(i);
        printTree(graph,&net,t,segment);
    }
}





bool IsIntree(node*v,std::unordered_map<node*,bool>&t1Point)
{  
    return t1Point.find(v)!=t1Point.end();
}
void LabelIntree(Graph*graph,NetGrids*net,node*v,std::unordered_map<node*,bool>&t1Point)
{
    while(v)
    {
        auto &grid = (*graph)(v->p.row,v->p.col,v->p.lay);
        if(t1Point.find(v)!=t1Point.end())break;
        t1Point.insert({v,true});
        // Enroll(grid,net); ////need
        net->PassGrid(&grid);
        v = v->parent;
    }
}
node* Search(Graph*graph,NetGrids*net,node *v,const pos&delta,std::unordered_map<std::string,float>&gridCost,tree*tmp)
{
    //Dir checking
    if(delta.row!=0&&v->p.lay%2==1){return nullptr;}//error routing dir
    if(delta.col!=0&&v->p.lay%2==0){return nullptr;}//error routing dir

    //Boundary checking
    pos P = {v->p.row+delta.row,v->p.col+delta.col,v->p.lay+delta.lay};
    if(P.row<graph->RowBound().first||P.row>graph->RowBound().second){return nullptr;} //RowBound checking
    if(P.col<graph->ColBound().first||P.col>graph->ColBound().second){return nullptr;} //ColBound checking

    int minLayer = graph->getNet(net->NetId).minLayer;
    if(P.lay<minLayer ||P.lay>graph->LayerNum()){return nullptr;}//minLayer checking
    
    //Capacity checking
    Ggrid& g = (*graph)(P.row,P.col,P.lay);
    bool enough = (net->AlreadyPass(&g))? true : g.get_remaining(); ////need
    if(!enough)return nullptr;


    //Caculate Cost    
    std::string str = pos2str(P);
    float lastCost = (gridCost.find(str)==gridCost.end())? FLT_MAX:gridCost[str];
    float pf = graph->getLay(P.lay).powerFactor;
    float weight = graph->getNet(net->NetId).weight;
    
    node * n = new node(P);
    n->cost = (net->AlreadyPass(&g))? 0:weight*pf+v->cost;////need

    if(n->cost<lastCost)
    {
        tmp->addNode(n);
        v->connect(n);
        gridCost[str] = n->cost;
    }
    else{
        delete n;
        n = nullptr;
    }
    return n;
}



void combine(tree*t1,tree*t2)
{
    for(auto l:t2->leaf)
    {
        t1->leaf.insert(l);
        l->routing_tree = t1;
    }
    for(auto n:t2->all)
    {
        t1->all.push_back(n);
        n->routing_tree = t1;
    }
    t2->all.clear();
    t2->leaf.clear();
    delete t2;
}
// tree* Tree2Tree(Graph*graph,Net*net,tree*t1,tree*t2)
// {
//     if(t1==t2)return t1;//precheck

//     //std::cout<<"Step1\n";
//     //Step 1 Setting Target&Source 
//     TargetTree(graph,net,t2);
//     SourceTree(graph,net,t1);
 

//     //std::cout<<"Step2\n";
//     //Step 2 Prepare Q
//     std::priority_queue<node*,std::vector<node*>,minCost>Q;
//     std::unordered_map<std::string,float>gridCost;
//     std::unordered_map<node*,bool>t1Point;//用來判斷isIntree
//     std::unordered_map<std::string,node*>t2Pesudo;//用來update two-pin-net sets...

//     //Multi Source
//     for(auto n:t1->all){//全部丟入Q 
//         n->cost = 0;
//         Q.push(n);
//         gridCost[pos2str(n->p)] = 0;
//         t1Point[n] = true;
//     }
//     bool t2IsPesudo = false;
//     for(auto n:t2->all){
//         if(n->p.lay==-1){ 
//             t2IsPesudo = true;
//             std::string twoDpos = std::to_string(n->p.row)+" "+std::to_string(n->p.col);
//             t2Pesudo[twoDpos] = n;
//         }  
//     }


//     //std::cout<<"Step3\n";
//     //Step 3 Search
//     node *targetPoint = nullptr;
//     tree* tmp = new tree;
//     while(!Q.empty()&&!targetPoint)
//     {
//         node * v = Q.top();Q.pop();
//         if((*graph)(v->p.row,v->p.col,v->p.lay).isTarget)//find
//         {
//             std::string twoDpos = std::to_string(v->p.row)+" "+std::to_string(v->p.col);
//             targetPoint = v;
//             if(t2Pesudo.find(twoDpos)!=t2Pesudo.end()){t2Pesudo[twoDpos]->p = v->p;}//updating pesudo-pin
//             break;
//         }
//         node * u = nullptr;
//         if(u = Search(graph,net,v,{0,0,1},gridCost,tmp))Q.push(u); //up
//         if(u = Search(graph,net,v,{0,0,-1},gridCost,tmp))Q.push(u);//down
//         if(u = Search(graph,net,v,{0,1,0},gridCost,tmp))Q.push(u);//-col
//         if(u = Search(graph,net,v,{0,-1,0},gridCost,tmp))Q.push(u);//+col
//         if(u = Search(graph,net,v,{1,0,0},gridCost,tmp))Q.push(u);//+row
//         if(u = Search(graph,net,v,{-1,0,0},gridCost,tmp))Q.push(u);//-row
//     }

//     //Untarget
//     UntargetTree(graph,net,t2);
//     if(t2IsPesudo)
//     {
//         tree *  pesudoClear = new tree;
//         for(auto pesudoPin:t2Pesudo)
//         {
//             node *pesudoNode = new node(pesudoPin.second->p); 
//             pesudoNode->p.lay = -1;
//             pesudoClear->addNode(pesudoNode);
//         }
//         UntargetTree(graph,net,pesudoClear);
//         delete pesudoClear;
//     }

//     //std::cout<<"Step4\n";//Step 4
//     if(targetPoint){
//         LabelIntree(graph,net,targetPoint,t1Point);
//         std::set<node*>recycle;
//         for(auto n:tmp->all)
//         {
//             if(!IsIntree(graph,net,n,t1Point)){
//                 recycle.insert(n);
//                 if(n->parent)
//                 tmp->leaf.insert(n->parent);
//             }
//         }
//         for(auto n:recycle){
//             tmp->all.remove(n);
//             tmp->leaf.erase(n);
//             for(auto c:n->child)
//             {
//                 if(c->parent==n)
//                     c->parent = nullptr;
//             }
//             delete n;
//         }
//         combine(t1,tmp);
//         combine(t1,t2);
//         return t1;
//     }
//     else {
//         delete tmp;
//         return nullptr;
//     }
// }
// std::pair<tree*,bool> Reroute(Graph*graph,Net*net,TwoPinNets&twopins)
// {
//     //std::cout<<"init"<<"\n";
//     int initdemand = TwoPinNetsInit(graph,net,twopins);//Init
//     //std::cout<<"ImitDmd =  "<<initdemand<<"\n";

//     tree*T= nullptr;
//     //std::cout<<"TwopinNet!\n";
//     for(auto pins:twopins)
//     {   
//         if(initdemand!=-1)
//             T = Tree2Tree(graph,net,pins.first->routing_tree,pins.second->routing_tree);
//         if(!T) //把整個two-pin nets 繞線產生出來的tree全部collect成一棵回傳
//         {
//             std::set<tree*>collect;//set(避免duplicate delete)
//             for(auto pins:twopins)
//             {
//                 collect.insert(pins.first->routing_tree);
//                 collect.insert(pins.second->routing_tree);
//             }
//             tree* CollectTree = new tree;
//             for(tree* t:collect)
//             {
//                 for(node * pin : t->leaf){CollectTree->leaf.insert(pin);}//for rip-up
//                 for(node * pin : t->all){CollectTree->all.push_front(pin);}//for delete
//             }
//             UnRegisterTree(graph,net,T);//need
//             return {CollectTree,false};
//         }
//     }
//     UnRegisterTree(graph,net,T);////need
//     AddingNet(graph,net,T);//need
//     return {T,true};
// }
