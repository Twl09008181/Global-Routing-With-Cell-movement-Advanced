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






void Enroll(Ggrid&grid,Net*net)
{
    grid.enrollNet = net;
}
void removedemand(Ggrid&grid,Net&net)
{
    if(grid.enrollNet==&net)
    {
        grid.delete_demand();
        Enroll(grid,nullptr);
    }
}
void addingdemand(Ggrid&grid,Net&net)
{
    if(grid.enrollNet!=&net)
    {
        grid.add_demand();
        Enroll(grid,&net);
    }
}
void RipUPinit(Graph*graph,Net&net)
{
    int NetId = std::stoi(net.netName.substr(1,-1));
    auto netTree = graph->getTree(NetId);
    for(auto grid:netTree->all)
    {
        pos p = grid->p;
        Enroll((*graph)(p.row,p.col,p.lay),&net);
    }
}
void RoutingInit(Graph*graph,Net&net,TwoPinNets&pinset)
{
    net.routingState = Net::state::routing;
    for(auto &twopin:pinset)
    {
        pos pin1 = twopin.first->p;
        pos pin2 = twopin.second->p;

        if(pin1.row==pin2.row&&pin1.col==pin2.col)
        {
            SegmentFun(graph,net,twopin.first,twopin.second,addingdemand);
        }
        else{
            if(pin1.lay!=-1){addingdemand((*graph)(pin1.row,pin1.col,pin1.lay),net);}
            if(pin2.lay!=-1){addingdemand((*graph)(pin2.row,pin2.col,pin2.lay),net);}
        }
    }
}

void SegmentFun(Graph*graph,Net&net,node*v,node*u,void(*f)(Ggrid&,Net&))
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

void Dfs_Segment(Graph*graph,Net&net,node*v,void(*f)(Ggrid&,Net&))
{
    v->mark = true;
    for(auto u:v->In)
    {
        if(u!=nullptr&&u->mark==false)
        {
            u->mark = true;
            SegmentFun(graph,net,v,u,f);
            Dfs_Segment(graph,net,u,f);
        }
    }
}

void RipUp(Graph*graph,Net&net,tree*t)
{
    if(net.routingState==Net::state::unroute)
    {
        std::cout<<"RipUP warning! "<<net.netName<<" Net.routing state = unroute, maybe this net does not allocate any demand on graph!!\n";
    }
    else if (net.routingState==Net::state::done)
    {

        for(auto n:t->all)
            n->mark = false;
            
        for(auto leaf:t->leaf){
            leaf->mark = true;
            Dfs_Segment(graph,net,leaf,removedemand);
        }
    }
}
void printgrid(Ggrid&g,Net&net)
{
    std::cout<<g.row<<" "<<g.col<<" "<<g.lay<<"\n";
}
void Unregister(Ggrid&g,Net&net)
{
    if(g.enrollNet!=&net)
    {
        std::cerr<<"error in Unregister, find some grid :"<<g.row<<" "<<g.col<<" "<<g.lay<<" is not enrolled by net:"<<net.netName<<"\n";
        exit(1);
        Enroll(g,nullptr);
    }
}

void printTree(Graph*graph,Net&net)
{
    int NetId = std::stoi(net.netName.substr(1,-1));
    tree * t = graph->getTree(NetId);
    for(auto n:t->all)
        n->mark = false;
    for(auto leaf:t->leaf){
        Dfs_Segment(graph,net,leaf,printgrid);
    }
}

void InitAddingDemand(Graph*graph,Net&net,tree*t)
{
    if(net.routingState==Net::state::done)
    {
        std::cout<<"InitAddingDemand warning! "<<net.netName<<" Net.routing state = done, this net may allocate duplicate times!!\n";
    }
    else if (net.routingState==Net::state::routing)
    {
        //std::cout<<net.netName<<"\n";
        for(auto n:t->all)
            n->mark = false;

        for(auto leaf:t->leaf){
            leaf->mark = true;
            Dfs_Segment(graph,net,leaf,addingdemand);
        }
    }
}

void doneAddingDemand(Graph*graph,Net&net,tree*t)
{
    for(auto n:t->all)
        n->mark = false;
    for(auto leaf:t->leaf){
        Dfs_Segment(graph,net,leaf,Unregister);
    }
}
