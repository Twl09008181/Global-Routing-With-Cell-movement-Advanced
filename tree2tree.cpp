#include <vector>
#include <utility>
#include <list>
#include <map>
#include <set>
#include <queue>
#include <string>
#include <iostream>
#include <algorithm>
#include <float.h>

struct pos{
    int row,col,lay;
};
std::ostream& operator<<(std::ostream&os,const pos&p1)
{
    os<<p1.row<<" "<<p1.col<<" "<<p1.lay<<" ";
    return os;
}
struct tree;
struct node{
    pos p;
    tree * routing_tree = nullptr;
    node* In[4] = {nullptr};
    bool IsLeaf = true;
    bool mark = false;//for print segment
    float cost = FLT_MAX;
    bool IsIntree = false;
};
struct tree{
    std::list<node*>leaf;
    std::list<node*>all;
};

void print_segment(node* n1,node*n2){std::cout<<n1->p<<" "<<n2->p<<"\n";}
void Dfs(node* n,void(*func)(node*,node*))
{
    n->mark = true;
    for(auto p:n->In)
    {
        if(p!=nullptr&&!p->mark){
            func(n,p);
            Dfs(p,func);
        }
    }
}

void print_tree(tree* T)
{
    for (auto n : T->all)
        n->mark = false;
    for (auto leaf:T->leaf)
        Dfs(leaf,print_segment);
}
bool isInTree(node*n,tree*t)
{
    // auto & t_p = t->all.front()->p;
    // if(t_p.lay==-1)//special case , tree t is only one unassignment node. 
    // {
    //     if(n->p.row==t_p.row&&n->p.col==t_p.col)
    //     {
    //         t_p.lay = n->p.lay;//lay assignment
    //         return true;
    //     }
    // }

    // else{
    //     for(auto node:t->all)
    //         if(n==node)return true;
    // }
    // return false;


}
void label(node*n1 ,node*n2)
{
    n1->IsIntree = n2->IsIntree = true;
}
void labelInTree(node*n){Dfs(n,label);}


// //dijkstra
// void do_relax(node*v,std::priority_queue<node*>&Q,std::map<std::string,node*>&Mark)
// {
//     v->p.row 

// }

//two-pin-net那邊應該要先把pin都放在前(排序)


tree * tree2tree(tree*T1,tree*T2)
{
    std::map<std::string,node*>Mark;
    std::priority_queue<node*>Q;//for dijkstra

    for(auto n : T2->all)
    {
        n->IsIntree = true;
    }

    for(auto n : T1->all){
        n->IsIntree = true;
        n->cost = 0;
        Q.push(n);
    }

    // node* connect = nullptr;
    // while(!Q.empty() && !connect)
    // {
    //     node* v = Q.top(); Q.pop();
    //     if(isInTree(v,T2))
    //     {
    //         v->IsIntree = true;
    //         connect = v;
    //         break;
    //     }
    //     //do relax
    //     //updating (Q,Mark)
    //     //
    //     //
    //     //
    // }

    // if(connect!=nullptr)
    // {
    //     labelInTree(connect);
    //     //combine T1,T2 leaf. 
    //     //combine T1,T2 all.
    // }
    // //clear those nodes not in tree.....
    // for(auto m:Mark)
    // {
    //     if(m.second.IsIntree==false)//手動清
    //         delete m.second.n;

    //     else if(connect!=nullptr)
    //         T1->all.push_front(m.second);
    // }

    // return (find) ? T1:nullptr;
}




#include "header/graph.hpp"
#include "header/Routing.hpp"
int LAYER_SEARCH_RANGE =3;
float ESCAPE_W = 10;
float VIA_W = 1;


Graph* graph = nullptr;


void Init(std::string path,std::string fileName);




using TwoPinNet = std::pair<node*,node*>;
void get_two_pins(std::list<TwoPinNet>& two_pin_nets,Net&net);

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

    std::cout<<"done!\n";
    for(auto net : graph->Nets){
        std::list<TwoPinNet> sets;
        get_two_pins(sets,*net.second);
        for(auto twopin:sets)
        {
            std::cout<<twopin.first->p<<" "<<twopin.second->p<<" \n";
        }
    }
    

	return 0;
}
void Init( std::string path,std::string fileName)
{
    graph = new Graph(path+fileName);
}



void two_pins_preprocessing(pos p,int rmax,int cmax,int LCstr,std::map<std::string,std::set<int>>&PointMap,std::vector<int>&tempDemand)
{
    std::string position_2d = std::to_string(p.row)+","+std::to_string(p.col);
    if(PointMap.find(position_2d) != PointMap.end())
    {
        PointMap[position_2d].insert(p.lay);
    }
    else
    {
        std::set<int>layRecord; layRecord.insert(p.lay);
        PointMap[position_2d] = layRecord; 
    }
    tempDemand.at((p.lay-1)*rmax + (p.row-1)*cmax + (p.col-1))+=1;
    if(p.lay<LCstr){//minlayer constraint
        auto & layRecord = PointMap[position_2d];
        layRecord.insert(LCstr);
        tempDemand.at((LCstr-1)*rmax + (p.row-1)*cmax + (p.col-1))+=1;
    }
}


int PinLayAssign(node*pin,int rmax,int cmax,int laymax,int LCstr,std::map<std::string,std::set<int>>&PointMap,std::vector<int>&tempDemand)
{
    std::string position_2d = std::to_string(pin->p.row)+","+std::to_string(pin->p.col);
    int bestLayer = 0;
    int mostcap = -INT_MAX;
    for(auto ptr = PointMap[position_2d].begin();ptr!=PointMap[position_2d].end();++ptr)//Scan layer
    {
        if(*ptr < LCstr)continue;//not a feasible solution
        int used = tempDemand.at((*ptr-1)*rmax + (pin->p.row-1)*cmax + (pin->p.col-1));
        if((*graph)(pin->p.row,pin->p.col,*ptr).get_remaining()-used > mostcap)
        {
            bestLayer = *ptr;
            mostcap = (*graph)(pin->p.row,pin->p.col,*ptr).get_remaining()-used;
        }
    }
    if(bestLayer==0)
    {
        std::cerr<<"PinLayAssign error!! , no remain capacity for "<<pin->p.row<<","<<pin->p.col<< "\n";
        exit(1);
    }
    return bestLayer;
}

void treeInit(std::map<std::string,node*>&pins,std::map<std::string,std::set<int>>&PointMap)
{
    for(auto &pin:pins)
    {
        pin.second->routing_tree = new tree();
        pin.second->routing_tree->all.push_back(pin.second);
        if(PointMap.find(pin.first)!=PointMap.end())//real-pin
        {
            for(auto ptr = PointMap[pin.first].begin();ptr!=PointMap[pin.first].end();++ptr)
            {
                if(*ptr!=pin.second->p.lay)
                {
                    node * n = new node();
                    n->p = pin.second->p;
                    n->p.lay = *ptr;
                    n->routing_tree = pin.second->routing_tree;
                    n->routing_tree->all.push_back(n);
                }
            }
        }
    }
}

void get_two_pins(std::list<TwoPinNet>& two_pin_nets,Net&net)
{
    
    int rowMax = graph->RowBound().second;
    int colMax = graph->ColBound().second;
    int LayMax = graph->LayerNum();

    std::vector<int>tempDemand(rowMax*colMax*LayMax);// (r,c,l) : index = (l-1)*rowMax+(r-1)*colMax + c-1
    int minLayer = net.minLayer;

    size_t len = net.net_pins.size();
    size_t real_len = 0;//some pins are at same row,col,lay.
    int row[len];
    int col[len];
    
    //Step1 : seting row,col array for flute
    //and using Point Map to record pins Info
    //1. tell if a pin is pseudo 
    //2. record those pins at same 2D-pos but different lay.
    
    std::map<std::string,std::set<int>>PointMap;
    //pseudo : is a steiner point.
    auto is_pseudo = [&PointMap](node*pin)
    {   
        std::string position_2d = std::to_string(pin->p.row)+","+std::to_string(pin->p.col);
        return PointMap.find(position_2d)==PointMap.end();
    };
    for(size_t i=0;i<len;++i){
        int r = net.net_pins.at(i).first->row;
        int c = net.net_pins.at(i).first->col;
        int l = net.net_pins.at(i).first->mCell->pins[net.net_pins.at(i).second];
        two_pins_preprocessing(pos{r,c,l},rowMax,colMax,minLayer,PointMap,tempDemand);
        row[real_len] = r;
        col[real_len] = c;
        real_len++;
    }

    //Step2 :Call flute to generate 2D steiner tree
    Tree t = flute(real_len, col, row, ACCURACY);//x,y
    
    std::map<std::string,node*>pins;//讓不同branch的相同pin同步,以免分配不同的lay
    auto search = [&pins](int row,int col)
    {
        std::string pin = std::to_string(row)+","+std::to_string(col);
        if(pins.find(pin)==pins.end())
        {
            pos p {row,col,-1};//暫時assign to -1
            node* n = new node();
            n->p = p;
            pins.insert({pin,n});
        }
        return pins[pin];
    };

    std::list<TwoPinNet>bothPseudo;
    //Step3 Init pins and do PinLayAssign
    for (int i=0; i<2*t.deg-2; i++) {
        int col1=t.branch[i].x;
        int col2=t.branch[t.branch[i].n].x;
        int row1=t.branch[i].y;
        int row2=t.branch[t.branch[i].n].y;
        node*pin1 = search(row1,col1);
        node*pin2 = search(row2,col2);
        if(pin1!=pin2)
        {
            if(!is_pseudo(pin1)&&pin1->p.lay==-1)//unassign
            {
                pin1->p.lay = PinLayAssign(pin1,rowMax,colMax,LayMax,minLayer,PointMap,tempDemand);
            }
            if(!is_pseudo(pin2)&&pin1->p.lay==-1)//unassign
            {
                pin2->p.lay = PinLayAssign(pin2,rowMax,colMax,LayMax,minLayer,PointMap,tempDemand);
            }
            if(pin1->p.lay==-1){std::swap(pin1,pin2);}//try to let pin1 be a real pin.
            //stil pseudo 
            if(pin1->p.lay==-1){bothPseudo.push_back({pin1,pin2});}
            else{two_pin_nets.push_back({pin1,pin2});}
        }
    }
    //Let pseudo be the last append
    for(auto psedoPins:bothPseudo){two_pin_nets.push_back(psedoPins);}

    //Step4 : generate Init routing tree and consider those who have same 2D-pos by using via. 
    treeInit(pins,PointMap);

    if(t.branch!=nullptr)
        free(t.branch);
}
