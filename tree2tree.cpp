
// void print_segment(node* n1,node*n2){std::cout<<n1->p<<" "<<n2->p<<"\n";}
// void Dfs(node* n,void(*func)(node*,node*))
// {
//     n->mark = true;
//     for(auto p:n->In)
//     {
//         if(p!=nullptr&&!p->mark){
//             func(n,p);
//             Dfs(p,func);
//         }
//     }
// }

// void print_tree(tree* T)
// {
//     for (auto n : T->all)
//         n->mark = false;
//     for (auto leaf:T->leaf)
//         Dfs(leaf,print_segment);
// }
// bool isInTree(node*n,tree*t)
// {
//     // auto & t_p = t->all.front()->p;
//     // if(t_p.lay==-1)//special case , tree t is only one unassignment node. 
//     // {
//     //     if(n->p.row==t_p.row&&n->p.col==t_p.col)
//     //     {
//     //         t_p.lay = n->p.lay;//lay assignment
//     //         return true;
//     //     }
//     // }

//     // else{
//     //     for(auto node:t->all)
//     //         if(n==node)return true;
//     // }
//     // return false;


// }
// void label(node*n1 ,node*n2)
// {
//     n1->IsIntree = n2->IsIntree = true;
// }
// void labelInTree(node*n){Dfs(n,label);}


// // //dijkstra
// // void do_relax(node*v,std::priority_queue<node*>&Q,std::map<std::string,node*>&Mark)
// // {
// //     v->p.row 

// // }

// //two-pin-net那邊應該要先把pin都放在前(排序)


// tree * tree2tree(tree*T1,tree*T2)
// {
//     std::map<std::string,node*>Mark;
//     std::priority_queue<node*>Q;//for dijkstra

//     for(auto n : T2->all)
//     {
//         n->IsIntree = true;
//     }

//     for(auto n : T1->all){
//         n->IsIntree = true;
//         n->cost = 0;
//         Q.push(n);
//     }

//     // node* connect = nullptr;
//     // while(!Q.empty() && !connect)
//     // {
//     //     node* v = Q.top(); Q.pop();
//     //     if(isInTree(v,T2))
//     //     {
//     //         v->IsIntree = true;
//     //         connect = v;
//     //         break;
//     //     }
//     //     //do relax
//     //     //updating (Q,Mark)
//     //     //
//     //     //
//     //     //
//     // }

//     // if(connect!=nullptr)
//     // {
//     //     labelInTree(connect);
//     //     //combine T1,T2 leaf. 
//     //     //combine T1,T2 all.
//     // }
//     // //clear those nodes not in tree.....
//     // for(auto m:Mark)
//     // {
//     //     if(m.second.IsIntree==false)//手動清
//     //         delete m.second.n;

//     //     else if(connect!=nullptr)
//     //         T1->all.push_front(m.second);
//     // }

//     // return (find) ? T1:nullptr;
// }




#include "header/graph.hpp"
#include "header/Routing.hpp"
#include "header/TwoPinNet.hpp"
int LAYER_SEARCH_RANGE =3;
float ESCAPE_W = 10;
float VIA_W = 1;


Graph* graph = nullptr;

void show_demand(Graph&graph);


void Init( std::string path,std::string fileName)
{
    graph = new Graph(path+fileName);
}

//void RipUPinit(Graph&graph,Net&net);//testing :因為現在init routing還沒改成tree,到時候要改成tree的結構Rip up
// void removedemandTesting(Graph&graph,Net&net);//testing :因為現在init routing還沒改成tree,到時候要改成tree的結構Rip up
void show_demand(Graph&graph);//for test
// void addingdemandTesting(Graph&graph,Net&net);//testing :因為現在init routing還沒改成tree,到時候要改成tree的結構Rip up




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


    //待新增destructor (clear all nodes&Inittree) if rip-up failed
    //待新增
    // for(int i = 1;i<=graph->Nets.size();i++){
    //     //std::cout<<"Net"<<i<<"\n";
    //     std::list<TwoPinNet> sets;
    //     auto &net = graph->getNet(i);
    //     get_two_pins(sets,net);
    //     for(auto twopin:sets)
    //     {
    //         std::cout<<twopin.first->p<<" "<<twopin.second->p<<" \n";
    //     }
    // }
    // std::cout<<"two-pin-net-done!!\n";
    //check1 pin要正確lay, pin要跟flute產生的一樣,pseudo 要是-1

    
    

    show_demand(*graph);

    // for(int i = 1;i<=graph->Nets.size();i++)
    // {
    //     tree * t = graph->getTree(i);
    //     std::cout<<"Net "<<i<<"\n";
    //     for(node*n:t->leaf)
    //     {
    //         std::cout<<"leaf : "<<n->p<<"\n";

    //         for(auto nb:n->In)
    //         {
    //             if(nb!=nullptr)
    //             {
    //                 std::cout<<nb->p<<" ";
    //             }
    //         }
    //         std::cout<<"\n";
    //     }
    // }

    for(int i = 1;i<=graph->Nets.size();i++){
        auto &net = graph->getNet(i); 
        net.routingState = Net::state::done;
        RipUPinit(graph,net);
        RipUp(graph,net,graph->getTree(i));
    }

    // for(int i = 1;i<=graph->Nets.size();i++){
    //     auto &net = graph->getNet(i); 
    //     printTree(graph,net);
    // }
    
    show_demand(*graph);
    

	return 0;
}




void show_demand(Graph&graph)
{
    int LayerNum = graph.LayerNum();
    std::pair<int,int>Row = graph.RowBound();
    std::pair<int,int>Col = graph.ColBound();

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
                total_demand+=graph(row,col,lay).demand;
            }
            //#ifdef PARSER_TEST
            //std::cout<<"\n";
            //#endif
        }
    }
    std::cout<<"Total :"<<total_demand<<"\n";
}


// //Call Enroll
// void RipUPinit(Graph&graph,Net&net)//testing :因為現在init routing還沒改成tree,到時候要改成tree的結構Rip up
// {
//     for(auto grid:*net.PassingGrids)
//     {
//         if(grid.second)
//         {
//             Enroll(*grid.first,&net);
//         }
//     }
// }
// void removedemandTesting(Graph&graph,Net&net)//testing :因為現在init routing還沒改成tree,到時候要改成tree的結構Rip up
// {
//     for(auto grid:*net.PassingGrids)
//     {
//         if(grid.second)
//         {
//             removedemand(*grid.first,net);
//         }
//     }
// }
// void addingdemandTesting(Graph&graph,Net&net)
// {
//     for(auto grid:*net.PassingGrids)
//     {
//         if(grid.second)
//         {
//             addingdemand(*grid.first,net);
//         }
//     }
// }
