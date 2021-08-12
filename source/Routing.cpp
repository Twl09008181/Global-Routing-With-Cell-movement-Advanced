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

bool Enroll(Ggrid&g,Net*net){
    g.enrollNet = net;
    return true;
}
bool Unregister(Ggrid&g,Net*net)
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
    if(grid.enrollNet==net)
    {
        grid.delete_demand();
        Enroll(grid,nullptr);
        return true;
    }
    return false;
}
bool addingdemand(Ggrid&grid,Net*net)
{
    if(grid.enrollNet!=net)
    {
        grid.add_demand();
        Enroll(grid,net);
        return true;
    }
    return false;
}
bool target(Ggrid&g,Net*net){g.isTarget = true;return true;}
//-------------------------------------------------callback functions------------------------------------------------------------



//--------------------------------------------------two-pin-sets function---------------------------------------------------
int TwoPinNetsInit(Graph*graph,Net*net,TwoPinNets&pinset)
{
    int totalInit = 0;
    for(auto &twopin:pinset)
    {
        pos pin1 = twopin.first->p;
        pos pin2 = twopin.second->p;
        if(pin1.lay!=-1){ (addingdemand((*graph)(pin1.row,pin1.col,pin1.lay),net)==true) ? totalInit+=1:totalInit+=0;}
        if(pin2.lay!=-1){(addingdemand((*graph)(pin2.row,pin2.col,pin2.lay),net)==true) ? totalInit+=1:totalInit+=0;}
    }
    net->routingState = Net::state::doneAdd;
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


struct Par
{
    std::string warning;//warning
    Net::state Stating[2];//[0]:invalid state , [1] :valid state , [2]: change state
    bool (*callback)(Ggrid&,Net*);
};
void RipUpAll(Graph*graph)
{
    for(int i = 1;i<=graph->Nets.size();i++){
        auto &net = graph->getNet(i); 
        TreeInterface(graph,&net,"RipUPinit");
        TreeInterface(graph,&net,"RipUP");
    }
}
void AddingAll(Graph*graph)
{
    for(int i = 1;i<=graph->Nets.size();i++){
        auto &net = graph->getNet(i);   
        TreeInterface(graph,&net,"Adding");
        TreeInterface(graph,&net,"doneAdd");
    }
}
void TreeInterface(Graph*graph,Net*net,const std::string &operation,tree* nettree)
{
    Par par;
    if(operation=="RipUPinit")
    {
        par.callback = Enroll;
        par.Stating[0] = Net::state::doneAdd;
        par.Stating[1] = Net::state::RipUpinit;
        par.warning    = "Warning : Net.routing state must be  Net::state::doneAdd!!\n";
    }
    else if (operation=="RipUP")
    {
        par.callback = removedemand;
        par.Stating[0] = Net::state::RipUpinit;
        par.Stating[1] = Net::state::CanAdd;
        par.warning    = "Warning : Net.routing state must be  Net::state::RipUPInit!!\n";
    }
    else if(operation=="Adding")
    {
        par.callback = addingdemand;
        par.Stating[0] = Net::state::CanAdd;
        par.Stating[1] = Net::state::Adding;
        par.warning    = "Warning : Net.routing state must be  Net::state::CanAdd!!\n";
    }
    else if (operation=="doneAdd")
    {
        par.callback = Unregister;
        par.Stating[0] = Net::state::Adding;
        par.Stating[1] = Net::state::doneAdd;
        par.warning    = " Warning : Net.routing state must be  Net::state::Adding!!\n";
    }
    else if (operation=="Enroll"){//can be used as RipUPinit but don't check the state.
        par.callback = Enroll;
        par.Stating[0] = Net::state::dontcare;
        par.Stating[1] = Net::state::RipUpinit;
    }
    else if (operation=="Unregister"){//can be used as doneAdd but don't check the state.
        par.callback = Unregister;
        par.Stating[0] = Net::state::dontcare;
        par.Stating[1] = Net::state::doneAdd;
    }
    else if (operation=="target")
    {
        par.callback = target;
        par.Stating[0] = Net::state::dontcare;
        par.Stating[1] = Net::state::dontcare;
    }
    else{
        std::cerr<<"Demand interface error!! unKnown op:"<<operation<<"\n";
        exit(1);
    }

    //-----core code

    tree *t = nullptr;
    if(nettree){
        t = nettree;
    }
    else
        t = graph->getTree(std::stoi(net->netName.substr(1,-1)));

    if(par.Stating[0]!=Net::state::dontcare && net->routingState!=par.Stating[0]){std::cerr<<net->netName<<par.warning;}
    else{
        // InStorage storage = getStorage(t);
        for(auto leaf:t->leaf){
            if(leaf->IsSingle()&&leaf->p.lay!=-1)
                par.callback((*graph)(leaf->p.row,leaf->p.col,leaf->p.lay),net);
            else if(leaf->p.lay!=-1)
                Backtack_Sgmt_Grid(graph,net,leaf,par.callback);
        }
        // RecoverIn(t,storage);
    }
    if(par.Stating[1]!=Net::state::dontcare)
        net->routingState = par.Stating[1];
}



//Output interface---------------------
void backTrackPrint(node*v,std::vector<std::string>*segment)
{
    while (v->parent)
    {
        std::string posv = pos2str(v->p);
        std::string posu = pos2str(v->parent->p);
        if(segment){segment->push_back(posu+" "+posv);}
        else{std::cout<<(posu+" "+posv)<<"\n";}
        v = v->parent;
    }
    
}

void printTree(tree*t,std::vector<std::string>*segment)
{
    for(auto leaf:t->leaf){
        if(!leaf->IsSingle()&&leaf->p.lay!=-1)
            backTrackPrint(leaf,segment);
    }
}
void PrintAll(Graph*graph,std::vector<std::string>*segment)
{
    for(int i = 1;i<=graph->Nets.size();i++){
        auto t = graph->getTree(i);
        printTree(t,segment);
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
