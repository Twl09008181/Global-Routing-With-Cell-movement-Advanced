#include "../header/Routing.hpp"

//INPUT: P1 , P2 which must only differ by one direction.
//Really adding demand into Ggrids. User must check the path not overflow.....
bool add_segment_3D(Ggrid&P1,Ggrid&P2,Graph&graph,int NetID)
{
    int differ_count = 3;
    int s_r = P1.row,t_r = P2.row;
    int s_c = P1.col,t_c = P2.col;
    int s_l = P1.lay,t_l = P2.lay;
    int d_r = (s_r != t_r) ? ((s_r < t_r)? 1 : -1) : 0;
    int d_c = (s_c != t_c) ? ((s_c < t_c)? 1 : -1) : 0;
    int d_l = (s_l != t_l) ? ((s_l < t_l)? 1 : -1) : 0;

    if(d_r==0)differ_count--;
    if(d_c==0)differ_count--;
    if(d_l==0)differ_count--;

    if(differ_count > 1)
    {
        std::cerr<<"Error input in void add_segment_3D(Point&P1,Point&P2,Graph&graph,int NetID)\n";
        exit(1);
    }

    Net& net = graph.getNet(NetID);
    do{
        //adding grid
        Ggrid& grid = graph(s_r,s_c,s_l);
        if(net.PassingGrid(grid)==false)return false;
        s_r += d_r;
        s_c += d_c;
        s_l += d_l;
    }while(s_r!=t_r||s_c!=t_c||s_l!=t_l);
    Ggrid& grid = graph(t_r,t_c,t_l);
    return net.PassingGrid(grid);
}



//功能:
//1. 判斷繞線至目標點(x,y,z)是否需要額外demand (若已經屬於net的一部分則不需要)
//2. 若需要額外demand,則判斷是否congestion 以及該Net是否能繞線至該目標點的layer
//first : CanRout or not
//second : need demand or not 
std::pair<bool,bool> CanRout(int row,int col,int lay,Graph&graph,int NetId)
{
    //check if this Point is valid for this Net first.
    auto RowRange = graph.RowBound();
    auto ColRange = graph.ColBound();
    int max_L = graph.LayerNum();
    int min_L = graph.getNet(NetId).minLayer;

    
    if(row < RowRange.first || col < ColRange.first || lay < min_L)return {false,false};//lowerbound check
    if(row > RowRange.second|| col > ColRange.second|| lay > max_L)return {false,false};//upperbound check

  
    //check if this Point has capacity to pass.
    Ggrid& grid = graph(row,col,lay);
    Net& net = graph.getNet(NetId);
    if(net.NotPass(grid)){
        return {grid.capacity > grid.demand,true};
    }
    else 
        return {true,false};//do not need one more demand

    // return {true,true};
}


//-------------------------------------------------node Member function-----------------------------------------------------------
bool node::IsLeaf()
{
    for(int i = 0;i<4;i++)
        if(Out[i])return false;
    return true;
}
bool node::IsSingle()//no In and no Out
{
    for(int i = 0;i<4;i++)
        if(In[i])return false;
    return this->IsLeaf();
}

void node::unregister(node*host)
{
    for(int i = 0;i<4;i++)
    {
        if(Out[i]==host){
            Out[i] = nullptr;
            host->In[i] = nullptr;
        }
    }
}

void node::connect(node *host)
{
    
    pos p_host = host->p;
    if(p.lay!=p_host.lay)
    {
        
        if(p_host.lay > p.lay)
        {
            node * oldclient = host->In[0];
            if(!oldclient||p.lay < oldclient->p.lay ){//updating downstream
                if(oldclient)
                    oldclient->unregister(host);
                host->In[0] = this;
                Out[0] = host;
            }
        }
        else{
            node * oldclient = host->In[1];
            if(!oldclient||p.lay > oldclient->p.lay ){//updating upstream
                if(oldclient)
                    oldclient->unregister(host);
                host->In[1] = this;
                Out[1] = host;
            }
        }
    }

    else if(p.col!=p_host.col)
    {
        if(p_host.col > p.col)
        {
            node * oldclient = host->In[2];
            if(!oldclient||p.col < oldclient->p.col ){//updating downstream
                if(oldclient)
                    oldclient->unregister(host);
                host->In[2] = this;
                Out[2] = host;
            }
        }
        else{
            node * oldclient = host->In[3];
            if(!oldclient||p.col > oldclient->p.col ){//updating upstream
                if(oldclient)
                    oldclient->unregister(host);
                host->In[3] = this;
                Out[3] = host;
            }
        }
    }

    else if(p.row!=p_host.row)
    {
        if(p_host.row > p.row)
        {
            node * oldclient = host->In[2];
            if(!oldclient||p.row < oldclient->p.row ){//updating downstream
                if(oldclient)
                    oldclient->unregister(host);
                host->In[2] = this;
                Out[2] = host;
            }
        }
        else{
            node * oldclient = host->In[3];
            if(!oldclient||p.row > oldclient->p.row ){//updating upstream
                if(oldclient)
                    oldclient->unregister(host);
                host->In[3] = this;
                Out[3] = host;
            }
        }
    }
        
}

//-------------------------------------------------node Member function-----------------------------------------------------------




//-------------------------------------------------callback functions------------------------------------------------------------
//format void(f)(Ggrid&,Net*)

bool Enroll(Ggrid&g,Net*net){
    if(g.enrollNet==nullptr||g.enrollNet==net||net==nullptr){
        g.enrollNet = net;
        return true;
    }
    else if(net){
        std::cerr<<net->netName<<" error in Enroll, find Ggrid :"<<g.row<<" "<<g.col<<" "<<g.lay<<" is already enrolled by net:"<<g.enrollNet->netName<<"\n";
        exit(1);
    }
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
//-------------------------------------------------callback functions------------------------------------------------------------



//--------------------------------------------------two-pin-sets function---------------------------------------------------
int TwoPinNetsInit(Graph*graph,Net*net,TwoPinNets&pinset)
{
    net->routingState = Net::state::Adding;
    int totalInit = 0;
    for(auto &twopin:pinset)
    {
        pos pin1 = twopin.first->p;
        pos pin2 = twopin.second->p;
        if(pin1.lay!=-1){ (addingdemand((*graph)(pin1.row,pin1.col,pin1.lay),net)==true) ? totalInit+=1:totalInit+=0;}
        if(pin2.lay!=-1){(addingdemand((*graph)(pin2.row,pin2.col,pin2.lay),net)==true) ? totalInit+=1:totalInit+=0;}
    }
    UnregisterNet(graph,net);
    return totalInit;
}



//--------------------------------------------------------DFS------------------------------------------------------------------
void SegmentFun(Graph*graph,Net*net,node*v,node*u,bool(*f)(Ggrid&,Net*))
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
    if(check>1){std::cerr<<"error in SementFun!!! Input: "<<u->p<<" " << v->p <<" is not a segment\n";exit(1);}
    do{
        auto &grid = (*graph)(sRow,sCol,sLay);
        f(grid,net);
        sRow+=d_r;
        sCol+=d_c;
        sLay+=d_l;
    }while(sRow!=tRow||sCol!=tCol||sLay!=tLay);
    auto &grid = (*graph)(sRow,sCol,sLay);
    f(grid,net);//last
}

void Dfs_Segment(Graph*graph,Net*net,node*v,bool(*f)(Ggrid&,Net*))
{
    for(int i = 0;i<4;i++)
    {
        node* u = v->In[i];
        if(u!=nullptr)
        {
            v->In[i] = nullptr;//edge dfs
            SegmentFun(graph,net,v,u,f);
            Dfs_Segment(graph,net,u,f);
        }
    }
}



//Edge dfs tool 
InStorage getStorage(tree*nettree)
{
    InStorage storage;
    storage.reserve(nettree->leaf.size());
    for(auto n:nettree->all){
        In In_Info = new node* [4];
        In_Info[0] = n->In[0];
        In_Info[1] = n->In[1];
        In_Info[2] = n->In[2];
        In_Info[3] = n->In[3];
        storage.push_back(In_Info);
    }
    return storage;
}
void RecoverIn(tree*nettree,InStorage&storage)
{
    int i = 0;
    for(auto n:nettree->all){
        for(int j = 0;j<4;j++)
            n->In[j] = storage.at(i)[j];
        delete [] storage.at(i);
        i++;
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
void TreeInterface(Graph*graph,Net*net,const std::string &operation)
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
    else{
        std::cerr<<"Demand interface error!! unKnown op:"<<operation<<"\n";
        exit(1);
    }

    //-----core code
    tree *t = graph->getTree(std::stoi(net->netName.substr(1,-1)));

    if(par.Stating[0]!=Net::state::dontcare && net->routingState!=par.Stating[0]){std::cerr<<net->netName<<par.warning;}
    else{
        InStorage storage = getStorage(t);
        for(auto leaf:t->leaf){
            if(leaf->IsSingle())
                par.callback((*graph)(leaf->p.row,leaf->p.col,leaf->p.lay),net);
            else
                Dfs_Segment(graph,net,leaf,par.callback);
        }
        RecoverIn(t,storage);
    }
    net->routingState = par.Stating[1];
}



//Output interface---------------------
void printTreedfs(node*v,std::vector<std::string>*segment)
{
    for(int i = 0;i<4;i++)
    {
        node* u = v->In[i];
        if(u!=nullptr)
        {
            v->In[i] = nullptr;//edge dfs
            std::string posv = pos2str(v->p);
            std::string posu = pos2str(u->p);
            if(segment){segment->push_back(posu+" "+posv);}
            else{std::cout<<(posu+" "+posv)<<"\n";}
            printTreedfs(u,segment);
        }
    }
}

void printTree(Graph*graph,Net*net,std::vector<std::string>*segment)
{
    int NetId = std::stoi(net->netName.substr(1,-1));
    tree * t = graph->getTree(NetId);

    //dfs init
    InStorage storage = getStorage(t);

    //dfs
    for(auto leaf:t->leaf){
        printTreedfs(leaf,segment);
    }
    //recover
    RecoverIn(t,storage);
}
void PrintAll(Graph*graph)
{
    for(int i = 1;i<=graph->Nets.size();i++){
        auto &net = graph->getNet(i); 
        printTree(graph,&net);
    }
}
