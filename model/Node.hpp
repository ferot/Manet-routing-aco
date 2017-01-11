#pragma once
#include <iostream>
#include <vector>
#include "RoutingEntry.hpp"
#include "Packet.h"

class RoutingEntry;

class Node: public std::enable_shared_from_this<Node> {

private:

std::vector<std::shared_ptr<RoutingEntry> > routingTable;

private:

    std::vector<std::shared_ptr<RoutingEntry> > findDestinationEntries(Packet packet);
    void startForwardAntPhase(int destinationAddress);
    void backwardAntPhase(int sourceAddress);

public:

    Node(){}
    Node(std::string name, int address);
    int address;
    std::string name;
    std::vector<std::shared_ptr<Node> > neighbours;

    void addNeighbour(std::shared_ptr<Node> node);
    void sendPacket(Packet packet);
    void passForwardAnt(int previousAddress, Packet ant);
    void updateRoutingEntry(Packet packet);
    std::shared_ptr<RoutingEntry> getEntryForDestinationAndHop(int dest, int hop);

};
