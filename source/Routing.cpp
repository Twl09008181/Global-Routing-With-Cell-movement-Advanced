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
//format bool(f)(Ggrid&,Net*)

bool EnrollNocheck(Ggrid&g,Net*net)
{
    g.enrollNet = net;
    return true;
}
bool Enroll(Ggrid&g,Net*net){
    if(net&&g.enrollNet!=net&&g.get_remaining()==0)//checking demand
    {
        std::cout<<"Warning ! Enroll : grid :"<<g.row<<","<<g.capacity<<","<<g.lay<<" no capacity!\n"<<std::endl;
        return false;
    }
    g.enrollNet = net;
    return true;
}
bool Unregister(Ggrid&g,Net*net)//整體繞線完
{
    if(g.enrollNet==net||g.enrollNet==nullptr||net==nullptr){
        return Enroll(g,nullptr);
    }
    else
    {
        std::cerr<<net->netName<<" error in Unregister, find Ggrid :"<<g.row<<" "<<g.col<<" "<<g.lay<<" is not enrolled by net:"<<net->netName<<"\n";
        exit(1);
    }
}
bool removedemand(Ggrid&grid,Net*net)
{
    if(grid.enrollNet==net)//要先確定被占用
    {
        grid.delete_demand();
        return Unregister(grid,net);
    }
    return false;
}
bool addingdemand(Ggrid&grid,Net*net)
{
    if(grid.enrollNet!=net)
    {
        if(Enroll(grid,net)){//try Enroll
            grid.add_demand();
            return true;
        }
    }
    return false;
}
//Tree2Tree
bool sourceInit(Ggrid&g,Net*net)
{
    g.isTarget = false;
    return Enroll(g,net);
}
bool target(Ggrid&g,Net*net){
    g.isTarget = true;    //結束後要設回來 或是t1在enroll要設定
    return Enroll(g,net);
}
bool Untarget(Ggrid&g,Net*net)
{
    g.isTarget = false;
    return true;
}
//-------------------------------------------------callback functions------------------------------------------------------------




//--------------------------------------------------two-pin-sets function---------------------------------------------------
int TwoPinNetsInit(Graph*graph,Net*net,TwoPinNets&pinset)
{
    int totalInit = 0;
    bool canInit = false;
    for(auto &twopin:pinset)
    {
        pos pin1 = twopin.first->p;
        pos pin2 = twopin.second->p;
        //std::cout<<"pin1!\n";
        if(pin1.lay!=-1){  (canInit =Enroll((*graph)(pin1.row,pin1.col,pin1.lay),net)==true) ? totalInit+=1:totalInit+=0;}
        //std::cout<<"canInit ="<<canInit<< "!\n";
        if(pin2.lay!=-1&&canInit){(canInit =Enroll((*graph)(pin2.row,pin2.col,pin2.lay),net)==true) ? totalInit+=1:totalInit+=0;}
        //std::cout<<"canInit ="<<canInit<< "!\n";
        if(!canInit){
            // std::cout<<"return -1!\n";
            return -1;
        }
        twopin.first->IsIntree = true;
        twopin.second->IsIntree = true;
    }
    net->routingState = Net::state::Routing;
    if(canInit)
        return totalInit;
    else 
        return -1;
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

void Sgmt_Grid(Graph*graph,Net*net,node*v,node*u,bool(*f)(Ggrid&,Net*))
{
    pos PosS,PosT,PosDelta;
    Sgmt_Init(v,u,PosS,PosT,PosDelta);
    do{
        auto &grid = (*graph)(PosS.row,PosS.col,PosS.lay);
        f(grid,net);
        PosS.row+=PosDelta.row;
        PosS.col+=PosDelta.col;
        PosS.lay+=PosDelta.lay;
    }while(PosS!=PosT);
    auto &grid = (*graph)(PosS.row,PosS.col,PosS.lay);
    f(grid,net);//last
}


void Backtack_Sgmt_Grid(Graph*graph,Net*net,node*v,bool(*f)(Ggrid&,Net*))
{
    while(v->parent)
    {
        Sgmt_Grid(graph,net,v,v->parent,f);
        v = v->parent;
    }
}
//------------------------------------------------Demand Interface------------------------------------------------------------------------


void RipUpAll(Graph*graph)
{
    
    for(int i = 1;i<=graph->Nets.size();i++){
        auto &net = graph->getNet(i); 
        RipUpNet(graph,&net);
    }
}
void AddingAll(Graph*graph)
{
    for(int i = 1;i<=graph->Nets.size();i++){
        auto &net = graph->getNet(i);   
        AddingNet(graph,&net);
    }
}
class minCost{
public:
    minCost() = default;
    bool operator()(const node* n1,const node*n2){return n1->cost > n2->cost;}
};
void TreeInterface(Graph*graph,Net*net,Par par,tree* nettree)
{
    tree *t = nullptr;
    if(nettree){
        t = nettree;
    }
    else
        t = graph->getTree(std::stoi(net->netName.substr(1,-1)));

    bool warningHappen = false;
    if(par.Stating[0]!=Net::state::dontcare && net->routingState!=par.Stating[0]){
        std::cerr<<net->netName<<par.warning<<" this operation will be discard!\n";
        warningHappen = true;
        exit(1);
    }
    else{
        for(auto leaf:t->leaf){
            if(leaf->IsSingle()&&leaf->p.lay!=-1)
                par.callback((*graph)(leaf->p.row,leaf->p.col,leaf->p.lay),net);
            else if(leaf->p.lay!=-1)
                Backtack_Sgmt_Grid(graph,net,leaf,par.callback);
            else if(leaf->p.lay==-1&&par.callback==target)//important
            {
                for(int i = net->minLayer;i<=graph->LayerNum();i++)//label all layer be target
                    (*graph)(leaf->p.row,leaf->p.col,i).isTarget = true;  
            }
            else if(leaf->p.lay==-1&&par.callback==Untarget)
            {
                for(int i = net->minLayer;i<=graph->LayerNum();i++)//label all layer be target
                    (*graph)(leaf->p.row,leaf->p.col,i).isTarget = false;  
            }
        }
    }
    if(par.Stating[1]!=Net::state::dontcare&&!warningHappen)
        net->routingState = par.Stating[1];
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
        TreeInterface(graph,&net,EnrollNocheck,t);

        printTree(graph,&net,t,segment);
    }
}



void Expand(node*v,node*u)
{
    pos PosS,PosT,PosDelta;
    Sgmt_Init(v,u,PosS,PosT,PosDelta);
    node*previous = v;
    v->IsIntree = true;//important
    u->IsIntree = true;//important

    while(PosS!=PosT){
        PosS.row+=PosDelta.row;
        PosS.col+=PosDelta.col;
        PosS.lay+=PosDelta.lay;
        //std::cout<<PosS<<"\n";
        if(PosS==PosT)
        {
            previous->connect(u);
            break;
        }
        node *ptr = new node(pos{PosS.row,PosS.col,PosS.lay});
        v->routing_tree->addNode(ptr);
        previous->connect(ptr);
        previous = ptr;
    }
}
void ExpandTree(tree*t)
{

    for(auto leaf:t->leaf){
        if(!leaf->IsSingle()&&leaf->p.lay!=-1)
        {
            while(leaf->parent)
            {
                //std::cout<<"leaf->parent loop\n";
                Expand(leaf->parent,leaf);
                leaf = leaf->parent;
            }
        }
    }
}







bool IsIntree(Graph*graph,Net*net,node*v,std::unordered_map<node*,bool>&t1Point)
{  
    return t1Point.find(v)!=t1Point.end();
}
void LabelIntree(Graph*graph,Net*net,node*v,std::unordered_map<node*,bool>&t1Point)
{
    while(v)
    {
        auto &grid = (*graph)(v->p.row,v->p.col,v->p.lay);
        if(t1Point.find(v)!=t1Point.end())break;
        t1Point.insert({v,true});
        Enroll(grid,net);
        v = v->parent;
    }
}
node* Search(Graph*graph,Net*net,node *v,const pos&delta,std::unordered_map<std::string,float>&gridCost,tree*tmp)
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
    n->cost = (g.enrollNet==net)? 0:weight*pf+v->cost;

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
tree* Tree2Tree(Graph*graph,Net*net,tree*t1,tree*t2)
{
    if(t1==t2)return t1;//precheck

    //std::cout<<"Step1\n";
    //Step 1 Setting Target&Source 
    TargetTree(graph,net,t2);
    SourceTree(graph,net,t1);
 

    //std::cout<<"Step2\n";
    //Step 2 Prepare Q
    std::priority_queue<node*,std::vector<node*>,minCost>Q;
    std::unordered_map<std::string,float>gridCost;
    std::unordered_map<node*,bool>t1Point;//用來判斷isIntree
    std::unordered_map<std::string,node*>t2Pesudo;//用來update two-pin-net sets...

    //Multi Source
    for(auto n:t1->all){//全部丟入Q 
        n->cost = 0;
        Q.push(n);
        gridCost[pos2str(n->p)] = 0;
        t1Point[n] = true;
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
    tree* tmp = new tree;
    while(!Q.empty()&&!targetPoint)
    {
        node * v = Q.top();Q.pop();
        if((*graph)(v->p.row,v->p.col,v->p.lay).isTarget)//find
        {
            std::string twoDpos = std::to_string(v->p.row)+" "+std::to_string(v->p.col);
            targetPoint = v;
            if(t2Pesudo.find(twoDpos)!=t2Pesudo.end()){t2Pesudo[twoDpos]->p = v->p;}//updating pesudo-pin
            break;
        }
        node * u = nullptr;
        if(u = Search(graph,net,v,{0,0,1},gridCost,tmp))Q.push(u); //up
        if(u = Search(graph,net,v,{0,0,-1},gridCost,tmp))Q.push(u);//down
        if(u = Search(graph,net,v,{0,1,0},gridCost,tmp))Q.push(u);//-col
        if(u = Search(graph,net,v,{0,-1,0},gridCost,tmp))Q.push(u);//+col
        if(u = Search(graph,net,v,{1,0,0},gridCost,tmp))Q.push(u);//+row
        if(u = Search(graph,net,v,{-1,0,0},gridCost,tmp))Q.push(u);//-row
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

    //std::cout<<"Step4\n";//Step 4
    if(targetPoint){
        LabelIntree(graph,net,targetPoint,t1Point);
        std::set<node*>recycle;
        for(auto n:tmp->all)
        {
            if(!IsIntree(graph,net,n,t1Point)){
                recycle.insert(n);
                if(n->parent)
                tmp->leaf.insert(n->parent);
            }
        }
        for(auto n:recycle){
            tmp->all.remove(n);
            tmp->leaf.erase(n);
            for(auto c:n->child)
            {
                if(c->parent==n)
                    c->parent = nullptr;
            }
            delete n;
        }
        combine(t1,tmp);
        combine(t1,t2);
        return t1;
    }
    else {
        delete tmp;
        return nullptr;
    }
}
std::pair<tree*,bool> Reroute(Graph*graph,Net*net,TwoPinNets&twopins)
{
    //std::cout<<"init"<<"\n";
    int initdemand = TwoPinNetsInit(graph,net,twopins);//Init
    //std::cout<<"ImitDmd =  "<<initdemand<<"\n";

    tree*T= nullptr;
    //std::cout<<"TwopinNet!\n";
    for(auto pins:twopins)
    {   
        if(initdemand!=-1)
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
            UnRegisterTree(graph,net,T);
            return {CollectTree,false};
        }
    }
    UnRegisterTree(graph,net,T);
    AddingNet(graph,net,T);
    return {T,true};
}
