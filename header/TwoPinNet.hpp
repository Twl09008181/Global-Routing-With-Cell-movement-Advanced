#ifndef TwoPinNet_HPP
#define TwoPinNet_HPP

#include "Routing.hpp"
#include "graph.hpp"

extern Graph* graph;

bool is_pseudo(node*pin,std::map<std::string,std::set<int>>&PointMap);
void two_pins_preprocessing(pos p,int rmax,int cmax,int LCstr,std::map<std::string,std::set<int>>&PointMap,std::vector<int>&tempDemand);
int PinLayAssign(node*pin,int rmax,int cmax,int laymax,int LCstr,std::map<std::string,std::set<int>>&PointMap,std::vector<int>&tempDemand);
void MultiLayerPoint(node*pin,std::map<std::string,std::set<int>>&PointMap);
void treeInit(std::map<std::string,node*>&pins,std::map<std::string,std::set<int>>&PointMap);

using TwoPinNet = std::pair<node*,node*>;
tree* get_two_pins(std::list<TwoPinNet>& two_pin_nets,Net&net);

bool two_pin_net_checker1(Tree *flute_tree,Net&net);//to check all 2d pin can be generate by flute and tell which pin is pseudo
bool two_pin_net_checker2(std::list<TwoPinNet>& two_pin_nets,tree* initTree,Net&net);//to check all 3d pin position can be covered by two_pin_nets and initTree


#endif
