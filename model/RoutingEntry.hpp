#pragma once
#include <vector>
#include <memory>
#include "Node.hpp"

class RoutingEntry {
private:

public:
    RoutingEntry(){}
    RoutingEntry (std::shared_ptr<Node> dest, std::shared_ptr<Node> nhop );
    std::shared_ptr<Node> destination;
    std::shared_ptr<Node> next_hop;

float pheromone;
};
