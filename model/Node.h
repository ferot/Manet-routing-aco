#include <iostream.h>
#include <vector>
#include "RoutingEntry.h"

class Node {

private:

std::vector<RoutingEntry> routing_table;

public:

Node(){}
int id;
std::vector<Node> neighbours;
};
