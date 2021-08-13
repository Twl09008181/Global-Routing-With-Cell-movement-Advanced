#include "../header/Routing.hpp"


bool node::IsSingle()//Is leaf and no parent
{
    return routing_tree->leaf.find(this)!=routing_tree->leaf.end()&&!parent;
}
void node::connect(node *host)
{
    if(this->parent!=host&&this!=host)
    {   
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
        if(pin1.lay!=-1){ canInit = (Enroll((*graph)(pin1.row,pin1.col,pin1.lay),net)==true) ? totalInit+=1:totalInit+=0;}
        if(pin2.lay!=-1&&canInit){canInit = (Enroll((*graph)(pin2.row,pin2.col,pin2.lay),net)==true) ? totalInit+=1:totalInit+=0;}
        if(!canInit)
            break;
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
void backTrackPrint(node*v,std::string &NetName,std::vector<std::string>*segment)
{
    while (v->parent)
    {
        std::string posv = pos2str(v->p);
        std::string posu = pos2str(v->parent->p);
        if(segment){segment->push_back(posu+" "+posv+" "+NetName);}
        else{std::cout<<(posu+" "+posv+" "+NetName)<<"\n";}
        v = v->parent;
    }
    
}

void printTree(tree*t,std::string &NetName,std::vector<std::string>*segment)
{
    for(auto leaf:t->leaf){
        if(!leaf->IsSingle()&&leaf->p.lay!=-1)
            backTrackPrint(leaf,NetName,segment);
    }
}
void PrintAll(Graph*graph,std::vector<std::string>*segment)
{
    for(int i = 1;i<=graph->Nets.size();i++){
        auto t = graph->getTree(i);
        printTree(t,graph->getNet(i).netName,segment);
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
