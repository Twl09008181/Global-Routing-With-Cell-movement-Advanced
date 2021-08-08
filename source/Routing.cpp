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


