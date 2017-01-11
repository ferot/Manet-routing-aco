#pragma once
#include <vector>
#include <memory>
#include "Node.hpp"

class Node;

class RoutingEntry {
private:

public:

    RoutingEntry() {}
    RoutingEntry(int dest, int nhop);

    int destinationAddress;
    int nextHopAddress;
    float pheromone;

};
