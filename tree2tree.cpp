
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

void show_demand(Graph*graph);

void Init( std::string path,std::string fileName)
{
    graph = new Graph(path+fileName);
}

void RoutingSchedule(Graph*graph);

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
    std::cout<<"Inital routing demand:\n";
    show_demand(graph);
    RoutingSchedule(graph);



    delete graph;
	return 0;
}

//routing interface必須有 繞線失敗就 recover demand 的功能
tree* Tree2Tree(Graph*graph,Net*net,tree*t1,tree*t2)
{
    bool success = false;//設定false來測試Rip up機制
    


    //routing procedure----------------------------------------

    //Step 1 dfs Enroll tree2 

    //Step 2 dijkstra   (還要設計cost function canroute等function)

    //Step 3 繞出來則設定sucess = true  然後  routing只存segment point (要backtrace 回去)

    //if failed, set success to false 


    //future : bounding box

    //routing procedure----------------------------------------


    //繞線完成後要將t2當中的leaf,all加入倒t1當中 並回傳t1 ,如果失敗就回傳nullptr
    if(success){
        
        //combine leaf node of t2 to t1 
        //combine all node of t2 to t1 
        
        //t2.leaf.clear()
        //t2.all.clear()
        return t1;
    }
    else {
        return nullptr;
    }
        
}
std::pair<tree*,bool> Reroute(Graph*graph,Net*net,TwoPinNets&twopins)
{
    int initdemand = TwoPinNetsInit(graph,net,twopins);//Init
    tree*T= nullptr;
    for(auto pins:twopins)
    {
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
            return {CollectTree,false};
        }
    }
    return {T,true};
}

void RoutingSchedule(Graph*graph)
{
    graph->placementInit();

    CellInst* movCell;
    int mov = 0;
    int success = 0;
    while( (movCell = graph->cellMoving()))//到時候可改多個Cell移動後再reroute,就把net用set收集後再一起reroute
    {
        mov++;
        bool movingsuccess = true;
        std::vector<tree*>netTrees(movCell->nets.size(),nullptr);

        //Reroute all net related to this Cell
        int lastFaileIdx = 0;
        for(int i = 0;i<movCell->nets.size();i++)
        {
            auto net = movCell->nets.at(i); 
            RipUpNet(graph,net);
            TwoPinNets twopins;
            get_two_pins(twopins,*net);
            auto result = Reroute(graph,net,twopins);
            netTrees.at(i) = result.first;
            if(result.second==false)
            {
                lastFaileIdx = i;
                movingsuccess = false;
                break;
            }
        }
        if(movingsuccess)//replace old tree
        {
            for(int i = 0;i<movCell->nets.size();i++)
            {
                int NetId = std::stoi(movCell->nets.at(i)->netName.substr(1,-1));
                graph->updateTree(NetId,netTrees.at(i));
            }
            success++;
        }
        else{///failed recover old tree demand

            //RipUp 這次繞線
            for(int i = 0;i<=lastFaileIdx;i++)
            {
                auto &net = movCell->nets.at(i);
                RipUpNet(graph,net,netTrees.at(i));//recover demand
            }

            for(int i = 0;i<=lastFaileIdx;i++){delete netTrees.at(i);}//delete 

            //重新復原demand
            for(int i = 0;i<=lastFaileIdx;i++)
            {
                auto &net = movCell->nets.at(i);
                AddingNet(graph,net);//recover demand
            }
        }
    }
    std::cout<<"final demand:\n";
    show_demand(graph);
    std::cout<<"count = "<<mov<<"\n";
    std::cout<<"sucess = "<<success<<"\n";
}



void show_demand(Graph*graph)
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
    std::cout<<"Total :"<<total_demand<<"\n";
}
