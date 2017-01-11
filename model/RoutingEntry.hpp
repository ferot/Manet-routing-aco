#pragma once
#include <vector>
#include <memory>
#include "Node.hpp"

class RoutingEntry {
private:

public:
RoutingEntry(){}

std::unique_ptr<Node> destination;
std::unique_ptr<Node> next_hop;

float pheromone;
};
