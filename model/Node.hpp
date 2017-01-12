#pragma once
#include <iostream>
#include <vector>
#include "RoutingEntry.hpp"
#include "Packet.h"

class RoutingEntry;

class Node: public std::enable_shared_from_this<Node> {

    typedef std::vector<std::shared_ptr<RoutingEntry> > tRoutingEntryVec;
    typedef std::vector<std::shared_ptr<Node> > tNodeVec;

private:

    std::vector<int> visitedForwardAnts;
    std::vector<int> visitedBackwardAnts;

private:

    tRoutingEntryVec findDestinationEntries(Packet packet);
    void startForwardAntPhase(int destinationAddress);
    void startBackwardAntPhase(Packet packet);

public:

    tRoutingEntryVec routingTable;

    Node(){}
    Node(std::string name, int address);
    int address;
    std::string name;
    tNodeVec neighbours;

    void addNeighbour(std::shared_ptr<Node> node);
    void sendPacket(Packet packet);
    void passForwardAnt(int previousAddress, Packet ant);
    void passBackwardAnt(int previousAddress, Packet ant);
    void updateRoutingEntry(Packet packet);
    std::shared_ptr<RoutingEntry> getEntryForDestinationAndHop(int dest, int hop);

};
