#ifndef ANALYSIS_HPP
#define ANALYSIS_HPP

#include "./data_structure.hpp"
#include "./TwoPinNet.hpp"
#include "./graph.hpp"
#include "./Routing.hpp"


#include <iostream>
#include <map>
#include <vector>
#include <string>
#include <climits>

inline int HPWL_flute(Net*net)
{

    int minR = INT_MAX;
    int maxR = 0;
    int minC = INT_MAX;
    int maxC = 0;
    int minL = INT_MAX;
    int maxL = 0;
    using std::min;
    using std::max;
    TwoPinNets twopins;
    get_two_pins(twopins,*net);
    for(auto pin:twopins)
    {
        int r1 = pin.first->p.row;
        int c1 = pin.first->p.col;
        int l1 = pin.first->p.lay;

        int r2 = pin.second->p.row;
        int c2 = pin.second->p.col;
        int l2 = pin.second->p.lay;

       
        minR = min(min(minR,r1),r2);
        maxR = max(max(maxR,r1),r2);

        minC = min(min(minC,c1),c2);

        maxC = max(max(maxC,c1),c2);

        if(l1!=-1){
            minL = min(minL,l1);
            maxL = max(maxL,l1);
        }
        if(l2!=-1)
        {
            minL = min(minL,l2);
            maxL = max(maxL,l2);
        }
    }

    int hpwl = maxR-minR + maxC-minC + maxL-minL;

    tree*collect = TwoPinNet_Collect(twopins);
    delete collect; 

    return hpwl;
}


inline int HPWL(Net*net)
{

    int minR = INT_MAX;
    int maxR = 0;
    int minC = INT_MAX;
    int maxC = 0;
    int minL = INT_MAX;
    int maxL = 0;
    using std::min;
    using std::max;
    for(auto pins:net->net_pins)
    {
        int r = pins.first->row;
        int c = pins.first->col;
        int l = pins.first->mCell->pins[pins.second];

        minR = min(r,minR);
        maxR = max(r,maxR);
        minC = min(c,minC);
        maxC = max(c,maxC);
        minL = min(l,minL);
        maxL = max(l,maxL);
    }
    int hpwl = maxR-minR + maxC-minC + maxL-minL;
    return hpwl;
}



inline void HPWL_distribution(Graph*graph,std::string fileName)
{
    fileName = fileName.substr(0,fileName.size()-4) + "_hpwl.txt";

    std::ofstream os{fileName};
    if(!os){
        std::cerr<<"error:file "<<fileName<<" cann't open!\n";
        exit(1);
    } 


    std::map<int,int>distribution;

    for(int i = 1;i<=graph->Nets.size();i++)
    {
        int hpwl = HPWL(&graph->getNet(i));
        if(distribution.find(hpwl)==distribution.end())
        {
            distribution[hpwl] = 1;
        }
        else{
            distribution.at(hpwl)+=1;
        }
    }


    os.width(10);
    os<<"------hpwl"<<"|"<<"-------num"<<"\n";
    os<<"----------"<<"-"<<"----------"<<"\n";

    for(auto d:distribution)
    {
        os.width(10);
        os<<d.first<<"|";
        os.width(10);
        os<<d.second<<"\n";
    }



    os.close();
}



//輸出每一層的使用率 以及使用的net數量 ,還有這些net的hwpl分布




inline void utilization(Graph*graph,std::string fileName)
{
    int maxR = graph->RowBound().second;
    int minR = graph->RowBound().first;
    int maxC = graph->ColBound().second;
    int minC = graph->ColBound().first;

    std::vector<float>util(graph->LayerNum(),0);
    for(int l = 1;l<=graph->LayerNum();l++)
    {
        auto &uti = graph->lay_uti(l);
        util.at(l-1) = static_cast<float>(uti.first)/uti.second *100;
    }
    
    std::vector<int>LayNets;
    LayNets.resize(graph->LayerNum()+1);
    for(auto net:graph->netGrids)
    {
        for(auto grid:net->grids)
        {
            LayNets.at(grid.first->lay)+=1;
        }
    }


    std::ofstream os{fileName.substr(0,fileName.size()-4)+"_ut.txt"};
    if(!os)
    {
        std::cerr<<"open error!\n";exit(1);
    }


    os<<"----------Layer"<<"|"<<"----utilization"<<"|"<<"------NumOfNets"<<"\n";
    os<<"---------------"<<"-"<<"---------------"<<"-"<<"---------------"<<"\n";

    os.width(15);
    
    for(int l = 1;l<=graph->LayerNum();l++)
    {
        os.width(15);
        os<<l;
        os.width(15);
        os<<util.at(l-1)<<"%";
        os.width(15);
        os<<LayNets.at(l)<<"\n";

    }
    
    os.close();

}




inline void OutPut(Graph*graph,std::string fileName)
{

    std::vector<std::string>segments;
    std::cout<<"Routing complete !\n";
    PrintAll(graph,&segments);
    //寫成輸出檔案
    int NumRoutes = segments.size();
    std::ofstream os{fileName};
    if(!os){
        std::cerr<<"error:file "<<fileName<<" cann't open!\n";
        exit(1);
    } 

    os<<"NumMovedCellInst "<< graph->moved_cells.size() <<"\n";
    for(auto cell:graph->moved_cells)
        os<<"CellInst "<< cell->name << " " << cell->row << " " << cell->col <<"\n";
    os<<"NumRoutes "<<NumRoutes<<"\n";

    for(auto s:segments)
    {
        os<<s<<"\n";
    }
    std::cout<<"saving done!\n";

    os.close();
}






#endif
