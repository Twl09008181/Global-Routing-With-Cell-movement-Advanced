#include "../header/graph.hpp"
#include "../header/Routing.hpp"



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
        PinsInit(*TMP);
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
            (*this)(row,col,b.first).demand += b.second;
        }
    }
    //------------------------------------------------------Initial Routing------------------------------------------------------------
    is >> type >> value;
    for(int i=0;i<value;++i){
        int r1,c1,l1;
        int r2,c2,l2;
        is >> r1 >> c1 >> l1 >> r2 >> c2 >> l2 >>type;
        int NetId = std::stoi(std::string(type.begin()+1,type.end()));
        Ggrid& g1 = (*this)(r1,c1,l1);
        Ggrid& g2 = (*this)(r2,c2,l2);
        add_segment_3D(g1, g2 ,*this,NetId);
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

//every time moving a cell,must first PinsRemov and PinsInit again
void Graph::PinsInit(Net&net)
{
  
    for(auto pins:net.net_pins)
    {
        CellInst* cell = pins.first;
        std::string name = pins.second;
        int lay = cell->mCell->pins[name];
        int row = cell->row;
        int col = cell->col;
        auto& grid = (*this)(row,col,lay);
        add_segment_3D(grid,grid,*this,std::stoi(net.netName.substr(1,-1)));
        //有可能是因為還沒把net移除會造成overflow (移動cell時沒辦法確定到時候被移動掉的net有哪些)
    }
}
void Graph::PinsRemov(Net&net)
{
    for(auto pins:net.net_pins)
    {
        CellInst* cell = pins.first;
        std::string name = pins.second;
        int lay = cell->mCell->pins[name];
        int row = cell->row;
        int col = cell->col;
        auto& grid = (*this)(row,col,lay);
        if(!net.NotPass(grid)){
            (*net.PassingGrids)[&grid] = false;
            grid.delete_demand();
        }
    }
}
void Graph::DoneReroute()
{
    for(auto net:Nets){
        net.second->RerouteFlag = false;
        delete net.second->PassingGridsTmp;
        net.second->PassingGridsTmp = nullptr;
    }
}
bool Graph::RipUpDemand(Net&net){
    if(net.RerouteFlag == true){return false;}//目前資料結構cellinst可能存重複的net,因此要避免重複reroute
    for(auto it = net.PassingGrids->begin();it!=net.PassingGrids->end();++it)
    {
        if(it->second==true){
            it->first->delete_demand();
            it->second = false;//暫時設為false
        }
    }
    net.PassingGridsTmp = new std::unordered_map<Ggrid*,bool>();
    net.PassingGridsTmp->clear();
    std::swap(net.PassingGrids,net.PassingGridsTmp);
    net.RerouteFlag = true;
    return true;
}
bool Graph::RecoverDemand(Net&net)
{
    for(auto it = net.PassingGrids->begin();it!=net.PassingGrids->end();++it)
    {
        if(it->second==false){
            it->first->demand++;
            it->second = true;
        }
    }
    // if(net.RerouteFlag == false)
    // {
    //     std::cerr<<"Warrning!! may overwrite the valid result,nust call RipUpDemand() first!!\n";
    // }


    for(auto grid = net.PassingGrids->begin();grid!=net.PassingGrids->end();grid++)
    {
        if(grid->second==true)
        {
            grid->first->delete_demand();
        }
    }

    std::swap(net.PassingGrids,net.PassingGridsTmp);
    delete net.PassingGridsTmp;//除此以外還要去把tmp之前造成的demand清乾淨



    net.PassingGridsTmp = nullptr;
    net.RerouteFlag = false;
}
