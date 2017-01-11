#pragma once
#include <iostream>
#include <vector>
#include "RoutingEntry.hpp"

class RoutingEntry;

class Node {

private:

std::vector<RoutingEntry> routing_table;

public:

    Node(){}
    Node(std::string name, int address);
    int address;
    std::string name;
    std::vector<std::shared_ptr<Node> > neighbours;

    void addNeighbour(std::shared_ptr<Node> node);
    void forwardAntPhase(int destinationAddress);

};
