#include "../header/TwoPinNet.hpp"
// void get_two_pins(TwoPinNets& two_pin_nets,Net&net);
// tree* TwoPinNet_Collect(TwoPinNets&twopins);
// bool is_pseudo(node*pin,std::unordered_map<std::string,std::set<int>>&PointMap);
// void two_pins_preprocessing(pos p,int rmax,int cmax,int LCstr,std::unordered_map<std::string,std::set<int>>&PointMap,std::vector<int>&tempDemand);
// int PinLayAssign(node*pin,int rmax,int cmax,int laymax,int LCstr,std::unordered_map<std::string,std::set<int>>&PointMap,std::vector<int>&tempDemand);
// void MultiLayerPoint(node*pin,std::unordered_map<std::string,std::set<int>>&PointMap,TwoPinNets& two_pin_nets);
// void treeInit(std::unordered_map<std::string,node*>&pins,std::unordered_map<std::string,std::set<int>>&PointMap,TwoPinNets& two_pin_nets);




bool is_pseudo(node*pin,std::unordered_map<std::string,std::set<int>>&PointMap)
{
    std::string position_2d = std::to_string(pin->p.row)+","+std::to_string(pin->p.col);
    return PointMap.find(position_2d)==PointMap.end();
}

int encode(int r,int c,int cmax)
{
    return r*cmax+c;
}
void updateTempDmd(int codePos,std::unordered_map<int,int>&tempDemand)
{
    auto temp = tempDemand.find(codePos);
    if(temp==tempDemand.end())
    {
        tempDemand.insert({codePos,1});
    }
    else{
        temp->second++;
    }
}

void two_pins_preprocessing(pos p,int rmax,int cmax,int LCstr,std::unordered_map<std::string,std::set<int>>&PointMap,std::unordered_map<int,int>&tempDemand)
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
    // tempDemand.at((p.lay-1)*rmax + (p.row-1)*cmax + (p.col-1))+=1;

    updateTempDmd(encode(p.row,p.col,cmax),tempDemand);


    if(p.lay<LCstr){//minlayer constraint
        auto & layRecord = PointMap[position_2d];
        layRecord.insert(LCstr);
        updateTempDmd(encode(p.row,p.col,LCstr),tempDemand);
    }
}


int PinLayAssign(node*pin,int rmax,int cmax,int laymax,int LCstr,std::unordered_map<std::string,std::set<int>>&PointMap,std::unordered_map<int,int>&tempDemand)
{
    std::string position_2d = std::to_string(pin->p.row)+","+std::to_string(pin->p.col);
    int bestLayer = 0;
    int mostcap = -2147483647;
    for(auto ptr = PointMap[position_2d].begin();ptr!=PointMap[position_2d].end();++ptr)//Scan layer
    {
        if(*ptr < LCstr)continue;//not a feasible solution
        // int used = tempDemand.at((*ptr-1)*rmax + (pin->p.row-1)*cmax + (pin->p.col-1));

        auto temp = tempDemand.find(encode(pin->p.row,pin->p.col,cmax));
        int used = (temp==tempDemand.end())? 0:temp->second;

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


void MultiLayerPoint(node*pin,std::unordered_map<std::string,std::set<int>>&PointMap,TwoPinNets& two_pin_nets)
{
    std::string position2d = std::to_string(pin->p.row)+","+std::to_string(pin->p.col);
    if(!is_pseudo(pin,PointMap))//real-pin
    {
        //copy and sort
        std::vector<int>sort_lay;sort_lay.resize(PointMap[position2d].size());
        std::copy(PointMap[position2d].begin(),PointMap[position2d].end(),sort_lay.begin());
        struct{bool operator()(int a,int b){return a<=b;}}cmp;
        std::sort(sort_lay.begin(),sort_lay.end(),cmp);
        //adding nodes into tree.
        for(int i = 1;i<=sort_lay.size();i++)
        {
            int last = sort_lay.at(i-1);
            int now = (i<sort_lay.size()) ? sort_lay.at(i):last+1;
            while(last<now)
            {
                node * n = nullptr;
                if(last != pin->p.lay)
                {
                    n = new node(pos{pin->p.row,pin->p.col,last});
                    if(n==nullptr)
                    {
                        std::cerr<<"MultiLayerPoint allocater failed!!\n";
                        exit(1);
                    }
                    pin->routing_tree->addNode(n);
                    pin->connect(n);
                }
                else{
                    n = pin;//for single grid pin
                }
                two_pin_nets.push_front({pin,n});
                last++;
            }
        }
    }
}


void treeInit(std::unordered_map<std::string,node*>&pins,std::unordered_map<std::string,std::set<int>>&PointMap,TwoPinNets& two_pin_nets)
{
    for(auto p:pins)
    {
        auto pin = p.second;
        std::string position2d = p.first;
        tree * nettree = new tree();
        if(nettree==nullptr){std::cerr<<"treeInit allocate error!!\n";exit(1);}
        nettree->addNode(pin);
        MultiLayerPoint(pin,PointMap,two_pin_nets);
    }
}


#include <chrono>

extern std::chrono::duration<double, std::milli> flutetime;

void get_two_pins(std::list<TwoPinNet>& two_pin_nets,Net&net)
{
    
    int rowMax = graph->RowBound().second;
    int colMax = graph->ColBound().second;
    int LayMax = graph->LayerNum();
    auto t1 = std::chrono::high_resolution_clock::now();
    // std::vector<int>tempDemand(rowMax*colMax*LayMax);// (r,c,l) : index = (l-1)*rowMax+(r-1)*colMax + c-1
    std::unordered_map<int,int>tempDemand;
    auto t2 = std::chrono::high_resolution_clock::now();
    flutetime+=t2-t1;
    int minLayer = net.minLayer;

    
    size_t len = net.net_pins.size();
    size_t real_len = 0;//some pins are at same row,col,lay.
    int row[len];
    int col[len];
    
    //Step1 : seting row,col array for flute
    //and using Point Map to record pins Info
    //1. tell if a pin is pseudo 
    //2. record those pins at same 2D-pos but different lay.
    std::unordered_map<std::string,std::set<int>>PointMap;
    
    for(size_t i=0;i<len;++i){
        int r = net.net_pins.at(i).first->row;
        int c = net.net_pins.at(i).first->col;
        int l = net.net_pins.at(i).first->mCell->pins[net.net_pins.at(i).second];
        
        two_pins_preprocessing(pos{r,c,l},rowMax,colMax,minLayer,PointMap,tempDemand);//updating PointMap&tempDemand
        
        
        row[real_len] = r;
        col[real_len] = c;
        real_len++;
    }


    //Step2 :Call flute to generate 2D steiner tree

    
    Tree t = flute(real_len, col, row, ACCURACY);//x,y



    std::unordered_map<std::string,node*>pins;//讓不同branch的相同pin同步,以免分配不同的lay
    auto search = [&pins](int row,int col)
    {
        std::string pin = std::to_string(row)+","+std::to_string(col);
        if(pins.find(pin)==pins.end())
        {
            node* n = new node(pos{row,col,-1});
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

        //std::cout<<row1<<" "<<col1<<" "<<row2<<" "<<col2<<"\n";
        node*pin1 = search(row1,col1);//unlayerassign
        node*pin2 = search(row2,col2);//unlayerassing
        if(pin1!=pin2)
        {
            if(!is_pseudo(pin1,PointMap)&&pin1->p.lay==-1)//unassign
            {
                pin1->p.lay = PinLayAssign(pin1,rowMax,colMax,LayMax,minLayer,PointMap,tempDemand);
            }
            if(!is_pseudo(pin2,PointMap)&&pin2->p.lay==-1)//unassign
            {
                pin2->p.lay = PinLayAssign(pin2,rowMax,colMax,LayMax,minLayer,PointMap,tempDemand);
            }
            if(pin1->p.lay==-1){std::swap(pin1,pin2);}//try to let pin1 be a real pin.
            //stil pseudo 
            if(pin1->p.lay==-1){bothPseudo.push_back({pin1,pin2});}
            else{two_pin_nets.push_back({pin1,pin2});}
        }
        if(pin1==pin2&&!is_pseudo(pin1,PointMap))//sometimes two-pins are at same Ggrid. 
        {
            //mulitPinInGrid.insert(pin1);
            if(pin1->p.lay==-1)
            pin1->p.lay = PinLayAssign(pin1,rowMax,colMax,LayMax,minLayer,PointMap,tempDemand);
        }
    }
     //Let pseudo be the last append (easy for routing)
    for(auto psedoPins:bothPseudo){two_pin_nets.push_back(psedoPins);}
    
    //Step4 : generate Init routing tree and consider those who have same 2D-pos by using via. 
    treeInit(pins,PointMap,two_pin_nets);

    if(t.branch!=nullptr)
        free(t.branch);


}


tree* TwoPinNet_Collect(TwoPinNets&twopins)
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
    return CollectTree;
}




//----------------------------------------------------Checker----------------------------------------------------------------------

bool two_pin_net_checker1(Tree *flute_tree,Net&net)
{

    std::map<std::string,bool>record;
    for(auto pin:net.net_pins)
    {
        int row = pin.first->row;
        int col = pin.first->col;
        std::string position_2d = std::to_string(row)+","+std::to_string(col);
        record.insert({position_2d,false});
    }

    std::list<std::string>steinerpoint;
    for (int i=0; i<2*flute_tree->deg-2; i++) {
        int col1=flute_tree->branch[i].x;
        int col2=flute_tree->branch[flute_tree->branch[i].n].x;
        int row1=flute_tree->branch[i].y;
        int row2=flute_tree->branch[flute_tree->branch[i].n].y;
        std::string p1 = std::to_string(row1)+","+std::to_string(col1);
        std::string p2 = std::to_string(row2)+","+std::to_string(col2);

        if(record.find(p1)!=record.end())
        {
            record[p1] = true;
        }
        else{
            steinerpoint.push_back(p1);
        }

        if(record.find(p2)!=record.end())
        {
            record[p2] = true;
        }
        else{
            steinerpoint.push_back(p2);
        }
    }

    for(auto pin:record)
    {
        if(pin.second==false)
        {
            std::cerr<<pin.first<<" not in flute!!\n";
            return false;
        }
    }

    for(auto st:steinerpoint)
    {
       // std::cout<<st<<" is a steiner point!\n";
    }
    return true;
}

bool two_pin_net_checker2(std::list<TwoPinNet>& two_pin_nets,Net&net)
{
    std::map<std::string,bool>record;
    for(auto pin:net.net_pins)
    {
        int row = pin.first->row;
        int col = pin.first->col;
        int lay = pin.first->mCell->pins[pin.second];
        std::string position_3d = std::to_string(row)+","+std::to_string(col)+","+std::to_string(lay);
        record.insert({position_3d,false});
    }

    //using tree to check 

    std::set<tree*>net_tree;
    for(auto pins:two_pin_nets)
    {
        net_tree.insert(pins.first->routing_tree);
        net_tree.insert(pins.second->routing_tree);
    }

    for(auto t:net_tree)
    {
        if(t==nullptr)
        {
            std::cerr<<"error!!!\n";
            exit(1);
        }
        for(auto n:t->all)
        {
            pos p = n->p;
            std::string position_3d = std::to_string(p.row)+","+std::to_string(p.col)+","+std::to_string(p.lay);
            if(record.find(position_3d)!=record.end())
                record[position_3d] = true;
        }
    }

    for(auto r:record)
    {
        if(r.second==false)
        {
            std::cerr<<r.first<<" not in two-pin-nets!!!\n";
            exit(1);
            return false;
        }
    }

    return true;
}
