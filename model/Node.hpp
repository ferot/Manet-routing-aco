#pragma once
#include <iostream>
#include <vector>
#include <utility>
#include "RoutingEntry.hpp"
#include "Packet.h"

class RoutingEntry;
class Node;

typedef std::vector<std::shared_ptr<RoutingEntry> > tRoutingEntryVec;
typedef std::vector<std::shared_ptr<Node> > tNodeVec;
typedef std::pair<std::shared_ptr<RoutingEntry>,float> tRentryProbPair;

class Node: public std::enable_shared_from_this<Node> {

private:

    std::vector<int> visitedForwardAnts;
    std::vector<int> visitedBackwardAnts;

private:

    tRoutingEntryVec findDestinationEntries(Packet packet);
    void startForwardAntPhase(int destinationAddress);
    void startBackwardAntPhase(Packet packet);
    std::shared_ptr<RoutingEntry> findBestPath (tRoutingEntryVec vec);
public:

    tRoutingEntryVec routingTable;

    Node(){}
    Node(std::string name, int address);
    int address;
    std::string name;
    tNodeVec neighbours;

    void addNeighbour(std::shared_ptr<Node> node);
    bool sendPacket(Packet packet);
    void passForwardAnt(int previousAddress, Packet ant);
    void passBackwardAnt(int previousAddress, Packet ant);
    void updateRoutingEntry(Packet packet);
    std::shared_ptr<RoutingEntry> getEntryForDestinationAndHop(int dest, int hop);

};
