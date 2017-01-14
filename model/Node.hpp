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

class Node: public std::enable_shared_from_this<Node> {

private:

    std::set<std::pair<int, int>> visitedAnts; // pair of ant sequence number and source hop
    std::vector<std::pair<int, tPacketptr>> entryBuffer;
    std::vector<std::pair<int, tPacketptr>> internalEntryBuffer;

private:

    tRoutingEntryVec findDestinationEntries(tPacketptr packet);
    std::shared_ptr<RoutingEntry> drawNextHop (tRoutingEntryVec vec);

    void passDiscoveryAnt(int previousAddress, tPacketptr ant);
    bool passRegularPacket(int previousAddress, tPacketptr packet);
    void passLoopPacket(tPacketptr packet);

public:

    tRoutingEntryVec routingTable;

    Node(){}
    Node(std::string name, int address);
    int address;
    std::string name;
    tNodeVec neighbours;

    void addNeighbour(std::shared_ptr<Node> node);

    void sendPacket(int fromHop, tPacketptr packet);
    void postTick();
    void tick();
    void increaseReverseEdgePheromone(int sourceAddress, int previousNode);

    void updateRoutingEntry(tPacketptr packet);
    std::shared_ptr<RoutingEntry> getEntryForDestinationAndHop(int dest, int hop);

};
