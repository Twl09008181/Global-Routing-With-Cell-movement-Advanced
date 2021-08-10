#include "../header/graph.hpp"
#include "../header/Routing.hpp"


void InitRoutingTree(Graph*graph,node*p1,node*p2,int netId);
//------------------------------------------------------Destructor-------------------------------------------------------------
Graph::~Graph(){}


//------------------------------------------------------Constructor--------------------------------------------------------------
Graph::Graph(std::string fileName){
    parser(fileName);
}

void Graph::parser(std::string fileName){
    std::ifstream is{fileName};
    if(!is){
        std::cerr<<"error:file "<<fileName<<" cann't open!\n";
        exit(1);
    }

    std::string type;
    int value;

    //cell max move
    is >> type >> MAX_Cell_MOVE;
    #ifdef PARSER_TEST
        std::cout<<type<<' '<<MAX_Cell_MOVE<<std::endl;
    #endif



    //GGridBoundaryIdx
    is >> type >> RowBegin >> ColBegin >> RowEnd >> ColEnd;
    #ifdef PARSER_TEST
        std::cout<<"GGridBoundaryIdx "<< RowBegin <<ColBegin << RowEnd << ColEnd<<std::endl;
    #endif
    //---------------------------------------------------Layer&Ggrid setting-------------------------------------------------------
    is >> type >> value;
    #ifdef PARSER_TEST
        std::cout<<type<<' '<< value << std::endl;
    #endif
    Layers.resize(value);//value is #Layers
    Ggrids.resize(value);
    for(int i = 0; i < value; ++i)
    {
        int index,supply;
        float powerfactor;
        //     Lay     M1      1        H       10       1.2
        is >> type >> type >> index >> type >> supply >> powerfactor;
        Layers.at(index-1) = Layer{type,supply,powerfactor};//Layer Info
        Ggrids.at(index-1) = Ggrid2D(RowEnd-RowBegin+1,Ggrid1D(ColEnd-ColBegin+1,Ggrid{supply}));//Layer Ggrids
    }
    #ifdef PARSER_TEST
        for(size_t i=0;i<Layers.size();++i){
            std::cout<<"M"<<i+1<<' ';
            if(Layers.at(i).horizontal)
                std::cout<<"H ";
            else
                std::cout<<"V ";
            std::cout << Layers.at(i).supply << ' ' << Layers.at(i).powerFactor<<std::endl;
        }
    #endif

    //----------------------------------------------------row/col/lay setting------------------------------------------------------
    for(int lay = 1;lay <= LayerNum();++lay)
    {
        for(int row = RowBegin; row <=RowEnd; ++row){
            for(int col = ColBegin; col<=ColEnd;++col)
            {
                (*this)(row,col,lay).lay = lay;
                (*this)(row,col,lay).row = row;
                (*this)(row,col,lay).col = col;
            }
        }
    }

    //NumNonDefaultSupplyGGrid

    is >> type >> value;
    #ifdef PARSER_TEST
        std::cout<<type<<' '<<value<<std::endl;
    #endif
    for(int i=0;i<value;++i){
        int row,col,lay,offset;
	    is >> row >> col >> lay >> offset;
	    (*this)(row,col,lay).capacity += offset;
        #ifdef PARSER_TEST
            std::cout<<row<<' '<<col<<' '<<lay<<' '<<offset<<std::endl;
        #endif
    }

    //-------------------------------------------------------MasterCell------------------------------------------------------------
    is >> type >> value;
    #ifdef PARSER_TEST
        std::cout<<type<<' '<<value<<std::endl;
    #endif
    for(int i = 0;i<value;i++){
        MasterCell *TMP = new MasterCell(is,mCell);
    }

    //-------------------------------------------------------CellInst------------------------------------------------------------
    is >> type >> value;
    for(int i = 0;i<value;i++){
        CellInst *TMP = new CellInst(is,mCell,CellInsts);
    }
    //----------------------------------------------------------Net------------------------------------------------------------
    is >> type >> value;
    for(int i = 0;i<value;i++){
        Net *TMP = new Net(is,CellInsts,Nets);
        //PinsInit(*TMP);//要改生成initRouting
    }

    //--------------------------------------------------------BlkgDemand---------------------------------------------------------------
    for (auto cell:CellInsts)//cell is string:value tpye
    {
        CellInst* c = cell.second;
        int row = c->row;
        int col = c->col;
        for (auto blkg:c->mCell->blkgs)//blkg is string : value type
        {
            MasterCell::Blkg b = blkg.second;
            //(*this)(row,col,b.first).demand += b.second;
        }
    }
    //------------------------------------------------------Initial Routing------------------------------------------------------------
    is >> type >> value;
    routingTree.reserve(Nets.size());
    for(int i = 1;i<=Nets.size();i++)
    {
        routingTree.push_back(new tree);
        if(this->getTree(i)==nullptr)
        {
            std::cerr<<"error!!\n";
            exit(1);
        }
    }
    std::vector<std::map<std::string,node*>>pins;

    pins.resize(Nets.size());
    auto searchnode = [&pins,this](int r,int c,int l,int netId)
    {
        std::string pos3d= std::to_string(r)+","+std::to_string(c)+","+std::to_string(l);
        auto&netnodes = pins.at(netId-1);
        auto nettree = this->getTree(netId);
        if(netnodes.find(pos3d)==netnodes.end())
        {
            node* n = new node();
            n->p = pos{r,c,l};
            netnodes.insert({pos3d,n});
            nettree->all.push_front(n);
            n->routing_tree = nettree;
            return n;
        }
        else
        {
            return netnodes[pos3d];
        }
    };

    //------------------------------------------------------------------------Testing Single grid pins-------------------------------------------------------
    //先把pin都記錄下來
    for(int i = 1;i<=Nets.size();i++)
    {
        auto &net = this->getNet(i);
        for(auto realpins:net.net_pins)
        {
            int r = realpins.first->row;
            int c = realpins.first->col;
            int l = realpins.first->mCell->pins[realpins.second];
            searchnode(r,c,l,i);
        }
    }
    std::vector<std::set<std::string>>SegmentPin;
    SegmentPin.resize(Nets.size());
    for(int i=0;i<value;++i){
        int r1,c1,l1;
        int r2,c2,l2;
        is >> r1 >> c1 >> l1 >> r2 >> c2 >> l2 >>type;
        int NetId = std::stoi(std::string(type.begin()+1,type.end()));

        //for testing single pin.----------------------------------
        std::string pos3d = std::to_string(r1)+","+std::to_string(c1)+","+std::to_string(l1);
        SegmentPin.at(NetId-1).insert(pos3d);
        pos3d = std::to_string(r2)+","+std::to_string(c2)+","+std::to_string(l2);
        SegmentPin.at(NetId-1).insert(pos3d);
        //for testing single pin.----------------------------------

        node*p1 = searchnode(r1,c1,l1,NetId);
        node*p2 = searchnode(r2,c2,l2,NetId);
        InitRoutingTree(this,p1,p2,NetId);
    }

    //此時會擁有single grid pin,就是leaf當中(leaf代表沒有指向別人) 沒有In的node
    std::vector<std::vector<std::string>>SingleGridPin;
    SingleGridPin.resize(Nets.size());
    for(int i = 1;i<=Nets.size();i++)
    {
        // std::cout<<"Net : "<<i<<"\n";
        for(auto n:this->getTree(i)->all)
        {
            int count = 0;
            for(auto in:n->In)
            {
                if(in)
                    count++;
            }
            if(n->IsLeaf==true&&count==0)
            {
                //std::cout<<n->p<<" is single grid pin!\n";
                std::string pos3d = std::to_string(n->p.row)+","+std::to_string(n->p.col)+","+std::to_string(n->p.lay);
                SingleGridPin.at(i-1).push_back(pos3d);
            }
        }
    }

    //讀取benchmark當中的pin
    //std::cout<<"reading bmark pin :\n";
    std::vector<std::map<std::string,bool>>benckMarkSinglePins;
    benckMarkSinglePins.resize(Nets.size());
    for(int i = 1;i<=Nets.size();i++)
    {
        //std::cout<<"Net"<<i<<"\n";
        auto &net = this->getNet(i);
        for(auto pin:net.net_pins)
        {
            CellInst * C = pin.first;
            int r =C->row;int c = C->col;int l = C->mCell->pins[pin.second];
            std::string pos3d = std::to_string(r)+","+std::to_string(c)+","+std::to_string(l);
            //std::cout<<pos3d<<"\n";

            if(SegmentPin.at(i-1).find(pos3d)==SegmentPin.at(i-1).end()&&benckMarkSinglePins.at(i-1).find(pos3d)==benckMarkSinglePins.at(i-1).end())
            {
                benckMarkSinglePins.at(i-1).insert({pos3d,false});
            }
        }
    }

    for(int i = 1;i<=Nets.size();i++)
    {
        //std::cout<<"Net"<<i<<"\n";
        for(auto singlepin : SingleGridPin.at(i-1))
        {
            if(benckMarkSinglePins.at(i-1).find(singlepin)==benckMarkSinglePins.at(i-1).end())
            {
                std::cerr<<"error !\n";
                exit(1);
            }

            benckMarkSinglePins.at(i-1)[singlepin]=true;
        }
        
        for(auto benchsinglepin : benckMarkSinglePins.at(i-1))
        {
            if(benchsinglepin.second==false)
            {
                std::cout<<"error!"<<benchsinglepin.first<<"\n";
                exit(1);
            }
        }
    }

    //------------------------------------------------------------------------Testing Single grid pins-------------------------------------------------------


    //LEAF node..
    for(int i = 1;i<=Nets.size();i++)
    {
        for(auto n:this->getTree(i)->all)
        {
            if(n->IsLeaf==true)
                this->getTree(i)->leaf.push_back(n);
        }
    }
    for(int i = 1;i<=Nets.size();i++)
    {
        auto &net = this->getNet(i);
        net.routingState = Net::state::routing;
        InitAddingDemand(this,net,this->getTree(i));
        net.routingState = Net::state::done;
        //doneAddingDemand(this,net,this->getTree(i));
    }
    
 
    //------------------------------------------------------NumVoltageAreas------------------------------------------------------------
    is >> type >> value;
    voltageAreas.resize(value);
    for(int i=0;i<value;++i){
        int counter=0;
        is >> type >> type >> type >> counter;
        for(int j=0;j<counter;++j){
            int g_x,g_y;
            is >> g_x >> g_y;
            voltageAreas.at(i).push_back({g_x,g_y});
        }
        is >> type >> counter;
        for(int j=0;j<counter;++j){
            is >> type;
            CellInsts[type]->vArea = i;//" from 0 to graph.cpp:voltageAreas.size()-1 "
        }
    }

    /*test
    for(auto it=CellInsts.begin();it!=CellInsts.end();++it){
        std::cout<<it->first<<std::endl;
        std::cout<<it->second->mCell<<std::endl;
        std::cout<<"row:"<<it->second->row<<std::endl;
        std::cout<<"col:"<<it->second->col<<std::endl;
        std::cout<<"Move:"<<it->second->Movable<<std::endl;
        std::cout<<"vArea"<<it->second->vArea<<std::endl<<std::endl;
    }*/


    is.close();
}


void Graph::showEffectedNetSize(){
	int counter = 0;
	std::vector<int> log10Count(10);
	for(auto& p : CellInsts){
		auto cellInst = p.second;
		if(cellInst->vArea == -1) continue;
		counter++;

		int leftBound, rightBound, upperBound, lowerBound;
		leftBound = lowerBound = INT32_MAX;
		rightBound = upperBound = INT32_MIN;

		for(auto net : cellInst->nets){	
			if(net->EndPoint[0]) leftBound = min(leftBound, net->EndPoint[0]->col);
			else break;
			rightBound = max(rightBound, net->EndPoint[1]->col);
			lowerBound = min(lowerBound, net->EndPoint[2]->row);
			upperBound = max(upperBound, net->EndPoint[3]->row);
		}
		
		log10Count[log10(leftBound != INT32_MAX ? (-leftBound + rightBound + 1) * (-lowerBound + upperBound + 1) : 1)]++;
	}

	for(int i=0; i<10; i++){
		std::cout << log10Count[i] << " ";
	}
	std::cout << std::endl;

}

CellInst* Graph::cellMoving(){
	// after the init each time pop one possible movement
	
	//check -> had not moved, (check coor)
	//		  enough capacity (this part not completely implement) 
	auto validMovement = [&](CellInst* cell, int voltageId){
		if(cell->row != cell->originalRow || cell->col != cell->originalCol)
			return false;
		
		std::vector<int> layersCurCap(LayerNum() + 1);
		for(int i = 1; i <= LayerNum(); i++){
			layersCurCap[i] = (*this) (cell->row, cell->col, i).get_remaining();
		}

		//std::vector< std::unordered_set<Net*>> layersNet(LayerNum() + 1);
		for(auto [name, pin] : cell->mCell->pins){
			//layersNet[pin].insert(name);
			//layersCurCap[pin] --;
		}

		for(auto [name, blkg] : cell->mCell->blkgs){
			layersCurCap[blkg.first] -= blkg.second;
		}

		for(auto curCap : layersCurCap){
			if(curCap < 0) return false;
		}

		return true;
	};

	while(candiPq.size()){
		auto [gain, cellId, voltageId] = candiPq.top();
		candiPq.pop();
		CellInst* cell = CellInsts["C" + std::to_string(cellId)];
		if(validMovement(cell, voltageId)){
			cell->row = voltageAreas[cell->vArea][voltageId].first;
			cell->col = voltageAreas[cell->vArea][voltageId].second;
			return cell;
		}
	}

	return nullptr;

}

void Graph::placementInit(){	
	for(auto& p : CellInsts){
		p.second->fixCell();
	}

	//updating fixed bounding box (Net)
	for(auto& p : Nets){
		p.second->updateFixedBoundingBox();
	}

	//updating optimal region (CellInst)
	for(auto& p : CellInsts){
		p.second->updateOptimalRegion();
	}	

	//calculate every possible movable position's grade 
	for(auto& p : CellInsts){
		CellInst* cPtr = p.second;
		if(!cPtr -> Movable) continue;
		
		int voltageType = cPtr->vArea;
		if(voltageType == -1) continue; //why not mova

		for(int i = 0; i < voltageAreas[voltageType].size(); i++){
			auto & coor = voltageAreas[voltageType][i];
			if(!cPtr->inOptimalRegion(coor.first, coor.second)) continue;
			
			int gain = 0;
			for(auto& net : cPtr->nets){
				gain += net->costToBox(cPtr->row, cPtr->col);
				gain -= net->costToBox(coor.first, coor.second);
			}

			//REMIND: gain / cell's index / grid index in the voltage
			if(gain >= 0) candiPq.push({gain, stoi(p.first.substr(1)), i });
		}
	}
}



void InitRoutingTree(Graph*graph,node*p1,node*p2,int netId)
{   
    p1->IsLeaf = false;//有outdegree就不是leaf
    pos pos1 = p1->p;
    pos pos2 = p2->p;
    //要添加In 方向.....
    if(pos1.lay!=pos2.lay){
        (pos2.lay>pos1.lay)? (p2->In[0] = p1) : (p2->In[1] = p1);
    }
    else if(pos1.col!=pos2.col)
    {
        (pos2.col>pos1.col)? (p2->In[2] = p1) : (p2->In[3] = p1);
    }    
    else if(pos1.row!=pos2.row)
    {
        (pos2.row>pos1.row)? (p2->In[2] = p1) : (p2->In[3] = p1);
    }
}

