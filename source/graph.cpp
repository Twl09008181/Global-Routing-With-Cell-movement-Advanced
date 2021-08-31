#include "../header/graph.hpp"
#include "../header/Routing.hpp"

std::vector<std::map<std::string,bool>>SegmentsTesting;
void InitRoutingTree(Graph*graph,node*p1,node*p2,int netId);
//------------------------------------------------------Destructor-------------------------------------------------------------


void DfsDestruct(node *v)
{



}

Graph::~Graph(){

    // std::cout<<"graph desturctor~\n";


    // std::cout<<"delete mCell~\n";
    for(auto mc:mCell)
    {
        delete mc.second;
    }
    // std::cout<<"delete CellInsts~\n";
    for(auto C:CellInsts)
    {
        delete C.second;
    }
    // std::cout<<"delete Nets~\n";
    for(auto N:Nets)
    {
        delete N.second;
    }
    // std::cout<<"delete routingTree~\n";
    for(auto t:routingTree)
    {
        delete t;
    }
    std::cout<<"graph destructor done!\n";
}


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

    //----------------------------------------------------utilization Record------------------------------------------------------
    utilizations.resize(LayerNum());
    for(int lay = 1;lay <= LayerNum();++lay)
    {
        int cap = 0;
        for(int row = RowBegin; row <=RowEnd; ++row){
            for(int col = ColBegin; col<=ColEnd;++col)
                cap+=(*this)(row,col,lay).capacity;
        }
        lay_uti(lay).second = cap;
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
    int totalBlkg = 0;
    for (auto cell:CellInsts)//cell is string:value tpye
    {
        CellInst* c = cell.second;
        int row = c->row;
        int col = c->col;
        for (auto blkg:c->mCell->blkgs)//blkg is string : value type
        {
            MasterCell::Blkg b = blkg.second;
            (*this)(row,col,b.first).add_demand(b.second);
            totalBlkg+=b.second;
            lay_uti(b.first).first += b.second;
        }
    }
    std::cout<<"Total Blkg = "<<totalBlkg<<"\n";
    //------------------------------------------------------Initial Routing------------------------------------------------------------
    is >> type >> value;
    routingTree.reserve(Nets.size());
    netGrids.reserve(Nets.size());
    for(int i = 1;i<=Nets.size();i++)
    {
        routingTree.push_back(new tree);
        netGrids.push_back(new NetGrids(i));
    }

    for(int i = 1;i<=Nets.size();i++)//預先讀取pin
    {
        auto &net = this->getNet(i);//NetId = i
        auto nettree = this->getTree(i);//NetId = i
        for(auto realpins:net.net_pins)
        {
            int r = realpins.first->row;
            int c = realpins.first->col;
            int l = realpins.first->mCell->pins[realpins.second];
            nettree->addNode(new node{pos{r,c,l}});
        }
    }
    //segment reading
    for(int i=0;i<value;++i){
        int r1,c1,l1;
        int r2,c2,l2;
        is >> r1 >> c1 >> l1 >> r2 >> c2 >> l2 >>type;
        int NetId = std::stoi(std::string(type.begin()+1,type.end()));
        auto nettree = this->getTree(NetId);
        auto netgrids = this->getNetGrids(NetId);
        node* p1 = new node{pos{r1,c1,l1}};
        node* p2 = new node{pos{r2,c2,l2}};
        nettree->addNode(p1);nettree->addNode(p2);
        p1->connect(p2);
    }
    for(int i = 1;i<=Nets.size();i++)
    {
        auto netgrid = this->getNetGrids(i);
        AddSegment(this,netgrid);
        AddingNet(this,netgrid);
    }
    for(auto t:routingTree)
    {
        t->updateEndPoint(this);
    }
    //------------------------------------------------voltage 
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
	for(const auto& p : CellInsts){
		auto cellInst = p.second;
		if(cellInst->vArea == -1) continue;
		counter++;

		int leftBound, rightBound, upperBound, lowerBound;
		leftBound = lowerBound = INT32_MAX;
		rightBound = upperBound = INT32_MIN;

        //這邊要update
		for(auto net : cellInst->nets){	
            int netId = std::stoi(net->netName.substr(1,-1));
            tree* nettree = this->getTree(netId);

			if(nettree->EndPoint[0]) leftBound = min(leftBound, nettree->EndPoint[0]->col);
			else break;
			rightBound = max(rightBound, nettree->EndPoint[1]->col);
			lowerBound = min(lowerBound, nettree->EndPoint[2]->row);
			upperBound = max(upperBound, nettree->EndPoint[3]->row);
		}
		
		log10Count[log10(leftBound != INT32_MAX ? (-leftBound + rightBound + 1) * (-lowerBound + upperBound + 1) : 1)]++;
	}

// 	for(int i=0; i<10; i++){
// 		std::cout << log10Count[i] << " ";
// 	}
// 	std::cout << std::endl;

}
std::pair<std::string,CellInst*> Graph::cellMoving(){
	// after the init each time pop one possible movement
	
	//check -> had not moved, (check coor)
	//		  enough capacity (this part not completely implement) 
	auto validMovement = [&](CellInst* cell, int curRow, int curCol){
		std::pair<int, int> coor = {curRow, curCol};
		if(coor.first == cell->row && coor.second == cell->col)
			return false;
	
		/*
		int gain = 0;
		for(auto& net : cell->nets){
			gain += net->costToBox(cell->row, cell->col) * 1.2;
			gain -= net->costToBox(coor.first, coor.second);
		}
		if(gain < 0) return false;
		*/


		std::vector<int> layersCurCap(LayerNum() + 1);
		for(int i = 1; i <= LayerNum(); i++){
			layersCurCap[i] = (*this) (cell->row, cell->col, i).get_remaining();
		}

		//std::vector< std::unordered_set<Net*>> layersNet(LayerNum() + 1);
		for(const auto& p : cell->mCell->pins){
			const auto& name = p.first;
			const auto& pin = p.second;
			
			
			//layersNet[pin].insert(name);
			layersCurCap[pin] --;
		}

		for(const auto& p : cell->mCell->blkgs){
			const auto& name = p.first;
			const auto& blkg = p.second;
			layersCurCap[blkg.first] -= blkg.second;
		}

		for(auto curCap : layersCurCap){
			if(curCap < 0) return false;
		}

		return true;
	};

	while(candiPq.size()){
		auto data = candiPq.top();
		candiPq.pop();
		//auto [priority, gain, cellId, curRow, curCol] = candiPq.top();
		int priority = std::get<0>(data);
		int gain = std::get<1>(data);
		int cellId = std::get<2>(data);
		int curRow = std::get<3>(data);
		int curCol = std::get<4>(data);
		
		
		CellInst* cell = CellInsts["C" + std::to_string(cellId)];

        //std::cout<<"Remov!\n";
        removeCellsBlkg(cell);

		if(validMovement(cell, curRow, curCol)){
			cell->row = curRow;
			cell->col = curCol;
            if(!insertCellsBlkg(cell)){
				//removeCellsBlkg(cell);
				cell->row = cell->originalRow;
				cell->col = cell->originalCol;
			}else return {"C" + std::to_string(cellId),cell};
		}
        insertCellsBlkg(cell);
	}

	return {"None",nullptr};

}

std::vector< std::pair<std::string,CellInst*>> Graph::cellSwapping(){
	auto validMovement = [&](CellInst* cell, int curRow, int curCol){
		std::pair<int, int> coor = {curRow, curCol};
		if(coor.first == cell->row && coor.second == cell->col)
			return false;
		std::vector<int> layersCurCap(LayerNum() + 1);
		for(int i = 1; i <= LayerNum(); i++){
			layersCurCap[i] = (*this) (cell->row, cell->col, i).get_remaining();
		}

		for(const auto& p : cell->mCell->pins){
			const auto& name = p.first;
			const auto& pin = p.second;
			

			layersCurCap[pin] --;
		}

		for(const auto& p : cell->mCell->blkgs){
			const auto& name = p.first;
			const auto& blkg = p.second;
			layersCurCap[blkg.first] -= blkg.second;
		}

		for(auto curCap : layersCurCap){
			if(curCap < 0) return false;
		}

		return true;
	};

	while(swappingCandiPq.size()){
		auto data = swappingCandiPq.top();
		swappingCandiPq.pop();
		//auto [gain, priority, cellId1, cellId2] = swappingCandiPq.top();
		int gain = std::get<0>(data);
		int priority = std::get<1>(data);
		int cellId1 = std::get<2>(data);
		int cellId2 = std::get<3>(data);

		
		
		CellInst* cell1 = CellInsts["C" + std::to_string(cellId1)];
		CellInst* cell2 = CellInsts["C" + std::to_string(cellId2)];

        //std::cout<<"Remov!\n";
        removeCellsBlkg(cell1);
        removeCellsBlkg(cell2);
		
		if(validMovement(cell1, cell2->row, cell2->col) & validMovement(cell2, cell1->row, cell1->col)){
			std::swap(cell1->row, cell2->row);
			std::swap(cell1->col, cell2->col);
            if(!insertCellsBlkg(cell1) | !insertCellsBlkg(cell2)){
				std::swap(cell1->row, cell2->row);
				std::swap(cell1->col, cell2->col);
			}else return {{"C" + std::to_string(cellId1), cell1}, {"C" + std::to_string(cellId2), cell2}};
		}
        insertCellsBlkg(cell1);
        insertCellsBlkg(cell2);
	}
	return std::vector< std::pair<std::string,CellInst*>> ();
}

void Graph::placementInit(){	
	if(movement_stage == 0){
		//updating fixed bounding box (Net)
		for(const auto& p : Nets){
			p.second->updateFixedBoundingBox();
		}

		//updating optimal region (CellInst)
		for(const auto& p : CellInsts){
			p.second->updateOptimalRegion();
			// insertCellsBlkg(p.second);
		}	
	}else{
		for(const auto& p : CellInsts){
			p.second->expandOptimalReion(movement_stage, RowBegin, RowEnd, ColBegin, ColEnd);
		}
	}
	movement_stage++;

	//calculate every possible movable position's grade 
	for(const auto& p : CellInsts){
		CellInst* cPtr = p.second;
		if(!cPtr -> Movable) continue;
		
		int voltageType = cPtr->vArea;
		if(voltageType == -1){
			//for(int i = 0; i < voltageAreas[voltageType].size(); i++){
			
			for(int curRow = RowBegin; curRow <= RowEnd; curRow++){
			for(int curCol = ColBegin; curCol <= ColEnd; curCol++){
				
				std::pair<int, int> coor = {curRow, curCol};
				if(!cPtr->inOptimalRegion(coor.first, coor.second)) continue;
				int priority = 0;// = cPtr->inOptimalRegion(coor.first, coor.second);

				int gain = 0;
				for(const auto& net : cPtr->nets){
					gain += net->costToBox(cPtr->row, cPtr->col);
					gain -= net->costToBox(coor.first, coor.second);
				}


				priority |= congest_value(cPtr->row, cPtr->col, 2) > 0.8 && congest_value(coor.first, coor.second, 2) < 0.5;	
				//REMIND: gain / cell's index / grid index in the voltage
				//if(gain >= 0)
					candiPq.push({priority,gain, stoi(p.first.substr(1)), curRow, curCol});
			}
			}
		}else{
			for(int i = 0; i < voltageAreas[voltageType].size(); i++){
				auto & coor = voltageAreas[voltageType][i];
				if(!cPtr->inOptimalRegion(coor.first, coor.second)) continue;
				int priority = 0;// = cPtr->inOptimalRegion(coor.first, coor.second);

				int gain = 0;
				for(const auto& net : cPtr->nets){
					gain += net->costToBox(cPtr->row, cPtr->col);
					gain -= net->costToBox(coor.first, coor.second);
				}


				priority |= congest_value(cPtr->row, cPtr->col, 2) > 0.8 && congest_value(coor.first, coor.second, 2) < 0.5;	
				//REMIND: gain / cell's index / grid index in the voltage
				//if(gain >= 0) 
					candiPq.push({priority,gain, stoi(p.first.substr(1)), coor.first, coor.second });
			}
		}
	}
}


void Graph::placementInit_Swap(){	
	//calculate every possible movable position's grade 
	
	if(CellInsts.size() <= 2) return ;
	for(auto it1 = CellInsts.begin(); it1 != CellInsts.end(); it1++){
		CellInst* cPtr1 = it1->second;
		if(!cPtr1 -> Movable) continue;
	for(auto it2 = next(it1); it2 != CellInsts.end(); it2++){
		CellInst* cPtr2 = it2->second;
		if(!cPtr2 -> Movable) continue;


		if(!cPtr1->inOptimalRegion(cPtr2->row, cPtr2->col) || 
		   !cPtr2->inOptimalRegion(cPtr1->row, cPtr1->col) ||
		   (cPtr1->vArea != -1 && !voltage_include[cPtr1->vArea].count(cPtr2->row << 16 + cPtr2->col)) ||
		   (cPtr2->vArea != -1 && !voltage_include[cPtr2->vArea].count(cPtr1->row << 16 + cPtr1->col)) ) continue;
		
	
		if(cPtr1->name == "C956"){
			std::cout << cPtr1->vArea << "!!!!!!!!!!!!!!!!!!!!";
		}

		int gain = 0;
		for(const auto& net : cPtr1->nets){
			gain += net->costToBox(cPtr1->row, cPtr1->col);
			gain -= net->costToBox(cPtr2->row, cPtr2->col);
		}
		for(const auto& net : cPtr2->nets){
			gain += net->costToBox(cPtr2->row, cPtr2->col);
			gain -= net->costToBox(cPtr1->row, cPtr1->col);
		}

		if(gain >= 0) swappingCandiPq.push({gain, gain, stoi(it1->first.substr(1)), stoi(it2->first.substr(1))});

	}
	}
}


double Graph::congest_value(int row, int col, int layer){
	const double matrix[3][3] = {0.075, 0.125, 0.075, 0.125, 0.2 , 0.125, 0.075, 0.125};
	double val = 0.0;
	for(int i = 0; i<3; i++){
		for(int j = 0; j < 3; j++){
			for(int l = 1; l <= layer; l++){
				val += matrix[i][j] * ((*this) (row, col, l).congestion_rate()) / layer;
			}
		}
	}
	return val;
}




bool Graph::removeCellsBlkg(CellInst* cell){	
	for(const auto& p : cell->mCell->blkgs){
		const auto& name = p.first;
		const auto& blkg = p.second;

		auto& grid = (*this)(cell->row, cell->col, blkg.first);
		grid.delete_demand(blkg.second);
		this->lay_uti(grid.lay).first-=blkg.second;
	}
	return true;
}

bool Graph::insertCellsBlkg(CellInst* cell){
	for(const auto& p : cell->mCell->blkgs){
		const auto& name = p.first;
		const auto& blkg = p.second;
		
		auto& grid = (*this)(cell->row, cell->col, blkg.first);
        if(grid.get_remaining()<blkg.second)return false;
		grid.add_demand(blkg.second);
		this->lay_uti(grid.lay).first+=blkg.second;
	}
	return true;
}



void Graph::show_cell_pos(){
	std::vector< std::vector<int>> table(RowEnd - RowBegin + 1, std::vector<int>(ColEnd - ColBegin + 1));
	//std::cout << RowBegin << " " << RowEnd << " " << ColBegin << " " << ColEnd << "\n";
	for(const auto& p : CellInsts){
		table[p.second->row - RowBegin][p.second->col - ColBegin]++;
		//std::cout << p.second->row - RowBegin << " " << p.second->col - ColBegin << std::endl;
	}
	for(const auto& v : table){
		for(const auto& n : v){
			std::cout << n << " ";
		}
		std::cout << "\n";
	}
}
void Graph::updateTree(int NetId,tree*t)
{
    if(NetId<1||NetId>Nets.size())
    {
        std::cout<<"void updateTree(int NetId,tree*t) input Error: 1<=NetId<="<<Nets.size()<<"\n";
        exit(1);
    }
    tree * oldtree = routingTree.at(NetId-1);
    delete oldtree;
    routingTree.at(NetId-1) = t;
    routingTree.at(NetId-1)->updateEndPoint(this);
}
void Graph::updateNetGrids(int NetId,NetGrids*netgrid)
{
    if(NetId<1||NetId>Nets.size())
    {
        std::cout<<"void updateNetGrids input Error: 1<=NetId<="<<Nets.size()<<"\n";
        exit(1);
    }
    NetGrids* oldnetgrid = netGrids.at(NetId-1);
    delete oldnetgrid;

    netGrids.at(NetId-1) = netgrid;
}
