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
            case Packet::Type::forward:
            case Packet::Type::backward:
                passDiscoveryAnt(fromNode, packet);
                break;
            case Packet::Type::regular:
                passRegularPacket(packet);
                break;
            case Packet::Type::route_error:
                break;
            case Packet::Type::duplicate_error:
                break;
            default:
                throw std::runtime_error("Unexpected ant type");
        }
    }
    internalEntryBuffer.clear();
}

void Node::passRegularPacket(tPacketptr packet)
{
    //we have to check if we have entries and if we are not in destination node - to avoid sending discovery
    if (address != packet->destinationAddress) {
        tRoutingEntryVec entries = findDestinationEntries(packet);

        if (entries.empty()) {
            cout<<"\n### Don't know where to send packet!!!\n";
        } 
        else {
            shared_ptr<RoutingEntry> bestPath = findBestPath(entries);
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

            cout<< "\n### Packet in node @address: " << address<< "\n Now sending packet to Node @address :" << bestNode->address << endl;
            bestNode->sendPacket(address, packet);
        }
    }
    else {
        cout<<"\n### Packet reached destination!!!\n";
    }
}

// Private

shared_ptr<RoutingEntry> Node::findBestPath (tRoutingEntryVec vec){
    float pheromoneSum = 0.0;
    shared_ptr<RoutingEntry> best = NULL;
    vector<tRentryProbPair> entryProb;

    //count sum of pheromones for available routes
    for_each(vec.begin(),vec.end(),[&pheromoneSum](shared_ptr<RoutingEntry> entry){
            pheromoneSum += entry->pheromone;
    });

    //count probability
    for_each(vec.begin(),vec.end(),[&](shared_ptr<RoutingEntry> entry){
        tRentryProbPair pair = make_pair(entry,entry->pheromone/pheromoneSum);
        entryProb.push_back(pair);
    });

    //sort in case the best route is unreachable, so that the latter entry should be considered
    sort(entryProb.begin(),entryProb.end(),[](const tRentryProbPair & a,const tRentryProbPair & b) -> bool {return (a.second > b.second);});
    best = (entryProb.front()).first;
    return best;
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

void Node::passDiscoveryAnt(int previousAddress, tPacketptr ant) {

    //TODO trzeba dodać obsługę błędu pętli
    std::cout << endl << "### Passing " << ant->type_string << " ant. Previous address " << previousAddress << " current address " << address << std::endl;

    std::shared_ptr<RoutingEntry> entry = getEntryForDestinationAndHop(ant->sourceAddress, previousAddress);
    if (entry == NULL) {
        cout << "@@@ Pushing entry onto Node's routingTable" << std::endl;
        entry = std::make_shared<RoutingEntry>(RoutingEntry(ant->sourceAddress, previousAddress));
        routingTable.push_back(entry);
        entry->increasePheromone(); //TODO wartość feromonów jest obliczana w zależności od liczby aktualnie wykonanych hopek (źródło: artykuł).
    }
    
    if(std::get<1>(visitedAnts.insert(ant->sequenceNumber))) { // if ant did not visit before

        if (ant->destinationAddress == address) {
            cout<< "### "<< ant->type_string <<" Ant finished its journey." << std::endl;
        } 
        else {
            for (auto neighbour : neighbours) {
                if (neighbour->address != previousAddress) {
                    std::cout << "Current " << address << " neighbour " << neighbour->address << " " << __FUNCTION__ << std::endl;
                    neighbour->sendPacket(address, ant);
                }
            }
        }
    } else {
        cout << "### Ignoring " << ant->type_string << " ant " << ant->sequenceNumber << " at Node with address " << address << endl;
    }

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
