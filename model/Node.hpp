#pragma once
#include <iostream>
#include <vector>
#include "RoutingEntry.hpp"

class Node {

private:

std::vector<RoutingEntry> routing_table;

public:

Node(){}
int id;
std::vector<Node> neighbours;
};
