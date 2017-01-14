#include "Node.hpp"
#include <memory>
#include <algorithm>
#include "Graph.h"

using namespace std;

// Constructors

Node::Node(std::string name, int address) : name(name), address(address) {}
// Public

void Node::addNeighbour(std::shared_ptr <Node> node) {

    auto it = find(neighbours.begin(), neighbours.end(), node);

	if(it == neighbours.end()) {
        neighbours.push_back(node);
    }
}

void Node::sendPacket(int fromHop, tPacketptr packet){
    packet->hops_count += 1;
    entryBuffer.push_back(std::make_pair(fromHop, packet));
}

void Node::postTick(){
    internalEntryBuffer = entryBuffer;
    entryBuffer.clear();
}

void Node::tick(){
    // evaporate pheromone
    for(auto entry: routingTable)
    {
        entry->evaporatePheromone();
    }

    // process incoming packets
    for (auto entryPair : internalEntryBuffer)
    {
        int fromNode = entryPair.first;
        tPacketptr packet = entryPair.second;

        switch (packet->type) {
            case Packet::Type::regular:
                if (!passRegularPacket(fromNode, packet)) {
                    tPacketptr forwardAnt = std::make_shared<Packet>(address, packet->destinationAddress, Packet::Type::forward);
                    sendPacket(address, forwardAnt);
                }
                break;
            case Packet::Type::forward:
            case Packet::Type::backward:
                passDiscoveryAnt(fromNode, packet);
                break;
            case Packet::Type::route_error:
                break;
            case Packet::Type::duplicate_error:
                passLoopPacket(packet);
                break;
            default:
                throw std::runtime_error("Unexpected ant type");
        }
    }
    internalEntryBuffer.clear();
}

bool Node::passRegularPacket(int previousAddress, tPacketptr packet)
{
    //we have to check if we have entries and if we are not in destination node - to avoid sending discovery
    if (address != packet->destinationAddress) {
        tRoutingEntryVec entries = findDestinationEntries(packet);

        if (entries.empty()) {
            cout<<"\n### Don't know where to send packet!!!\n";
            return false;
        } 
        else {
            shared_ptr<RoutingEntry> bestPath = drawNextHop(entries);
            shared_ptr<Node> bestNode = NULL;

            //find the best node by address
            for(auto node : neighbours){
                if(node->address == bestPath->nextHopAddress){
                    bestNode = node;
                    break;
                }
            }

            //TODO jeszcze trzeba zwiększyć feromony krawędzi, z której paczka przyszła
            bestPath->increasePheromone();
            bestNode->increaseReverseEdgePheromone(packet->sourceAddress, address);

            cout<< "\n### Packet in node @address: " << address<< "\n Now sending packet to Node @address :" << bestNode->address << endl;
            bestNode->sendPacket(address, packet);
        }
    }
    else {
        cout<<"\n### Packet reached destination!!!\n";
    }

    return true;
}

// Private

shared_ptr<RoutingEntry> Node::drawNextHop (tRoutingEntryVec vec){

    float pheromoneSum = 0.0;

    //count sum of pheromones for available routes
    for_each(vec.begin(),vec.end(),[&pheromoneSum](shared_ptr<RoutingEntry> entry){
            pheromoneSum += entry->pheromone;
    });

    double random = ((float)rand()/(float)RAND_MAX) * pheromoneSum;

    int choosenHop = -1;
    while(random >= 0.0) {
        ++choosenHop;
        random -= vec[choosenHop]->pheromone;
    }

    return vec[choosenHop];
}

void Node::passDiscoveryAnt(int previousAddress, tPacketptr ant) {

    std::cout << endl << "### Passing " << ant->type_string << " ant. Previous address " << previousAddress << " current address " << address << std::endl;

    std::shared_ptr<RoutingEntry> entry = getEntryForDestinationAndHop(ant->sourceAddress, previousAddress);
    if (entry == NULL && ant->sourceAddress != address) { //TODO czy wystarczająco zapobiega to powstawaniu pętli? W artykule jest trochę inaczej
        cout << "@@@ Pushing entry onto Node's routingTable" << std::endl;
        entry = std::make_shared<RoutingEntry>(RoutingEntry(ant->sourceAddress, previousAddress));
        routingTable.push_back(entry);
        entry->increasePheromone(); //TODO wartość feromonów jest obliczana w zależności od liczby aktualnie wykonanych hopek (źródło: artykuł).
    }
    
    if(std::get<1>(visitedAnts.insert(std::make_pair(ant->sequenceNumber, previousAddress)))) { // if ant did not visit before
        if (ant->destinationAddress == address) {
            cout<< "### "<< ant->type_string <<" Ant finished its journey." << std::endl;
            if (ant->type == Packet::Type::forward) {
                tPacketptr backwardAnt = std::make_shared<Packet>(address, ant->sourceAddress, Packet::Type::backward);
                sendPacket(address, backwardAnt);
            }
        } 
//        else {
            for (auto neighbour : neighbours) {
                if (neighbour->address != previousAddress) {
                    std::cout << "Current " << address << " neighbour " << neighbour->address << " " << __FUNCTION__ << std::endl;
                    neighbour->sendPacket(address, ant);
                }
            }
//        }
    } else {
        cout << "### Ignoring " << ant->type_string << " ant " << ant->sequenceNumber << " at Node with address " << address << endl;
    }
}

void Node::passLoopPacket(tPacketptr packet) {
    if (packet->destinationAddress == address)
    {
        //Delete looping route

        tRoutingEntryVec::iterator it;
        for (it = routingTable.begin(); it != routingTable.end(); ++it) {
            if ((*it)->destinationAddress == packet->destinationAddress && 
                (*it)->nextHopAddress == packet->sourceAddress) 
            {
                break;
            }
        }

        if (it == routingTable.end())
        {
            throw std::runtime_error("Routing entry to delete was not found.");
        }

        routingTable.erase(it);
    }
    else
    {
        throw std::runtime_error("Received duplicate_error that was not addressed to me.");
    }
}

tRoutingEntryVec Node::findDestinationEntries(tPacketptr packet) {
    tRoutingEntryVec entries;

    for_each(routingTable.begin(), routingTable.end(), [&](std::shared_ptr<RoutingEntry> entry) {
        if (entry->destinationAddress == packet->destinationAddress) {
            entries.push_back(entry);
        }
    });

    return entries;
}

std::shared_ptr<RoutingEntry> Node::getEntryForDestinationAndHop(int dest, int hop) {
    std::shared_ptr<RoutingEntry> entry = NULL;

    std::for_each(routingTable.begin(), routingTable.end(), [&](std::shared_ptr<RoutingEntry> routingEntry) {
        if (routingEntry->destinationAddress == dest && routingEntry->nextHopAddress == hop) {
            entry = routingEntry;
        }
    });

    return entry;
}

void Node::increaseReverseEdgePheromone(int sourceAddress, int previousNode) {
    std::shared_ptr<RoutingEntry> entry = getEntryForDestinationAndHop(sourceAddress, previousNode);
    if (entry == NULL) {
        entry = std::make_shared<RoutingEntry>(RoutingEntry(sourceAddress, previousNode));
        routingTable.push_back(entry);
        entry->increasePheromone();
    } else {
        entry->increasePheromone();
    }
}