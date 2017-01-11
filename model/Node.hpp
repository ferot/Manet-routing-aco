#pragma once
#include <iostream>
#include <vector>
#include "RoutingEntry.hpp"
#include "Packet.h"

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
    void sendPacket(Packet packet);
    void forwardAntPhase(int destinationAddress);

};
