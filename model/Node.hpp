#pragma once
#include <iostream>
#include <vector>
#include <set>
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

    std::set<int> visitedAnts;

private:

    tRoutingEntryVec findDestinationEntries(tPacketptr packet);
    std::shared_ptr<RoutingEntry> findBestPath (tRoutingEntryVec vec);
public:

    tRoutingEntryVec routingTable;

    Node(){}
    Node(std::string name, int address);
    int address;
    std::string name;
    tNodeVec neighbours;

    void addNeighbour(std::shared_ptr<Node> node);
    bool sendPacket(tPacketptr packet);
    void startAntDiscoveryPhase(tPacketptr packet);
    void passDiscoveryAnt(int previousAddress, tPacketptr ant);
    void updateRoutingEntry(tPacketptr packet);
    std::shared_ptr<RoutingEntry> getEntryForDestinationAndHop(int dest, int hop);

};
