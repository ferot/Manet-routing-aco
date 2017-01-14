#include "Node.hpp"
#include <memory>
#include <algorithm>
#include "Graph.h"

using namespace std;

extern Graph graph;

static void updatePherNodes(){
    tNodeVec nodes = graph.nodes;

    for_each(nodes.begin(),nodes.end(),[](shared_ptr<Node> &node){
        for(auto entry: node->routingTable)
        {
            entry->evaporatePheromone();
        }
    });
}

// Constructors

Node::Node(std::string name, int address) : name(name), address(address) {}
// Public

void Node::addNeighbour(std::shared_ptr <Node> node) {

    auto it = find(neighbours.begin(), neighbours.end(), node);

	if(it == neighbours.end()) {
        neighbours.push_back(node);
    }
}

bool Node::sendPacket(tPacketptr packet) {

    updatePherNodes();

    //we have to check if we have entries and if we are not in destination node - to avoid sending discovery
    if (address != packet->destinationAddress) {
        tRoutingEntryVec entries = findDestinationEntries(packet);

        if (entries.empty()) {
            return false;
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
            return bestNode->sendPacket(packet);
        }
    }
    else {
        cout<<"\n### Packet reached destination!!!\n";
        return true;
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

void Node::startAntDiscoveryPhase(tPacketptr ant) {

    if(std::get<1>(visitedAnts.insert(ant->sequenceNumber))) {
        for (auto neighbour : neighbours) {
            neighbour->passDiscoveryAnt(address, ant);
        }
    }
}

void Node::passDiscoveryAnt(int previousAddress, tPacketptr ant) {

    //TODO trzeba dodać obsługę pętli
    //TODO sposób przeszukiwania powoduje, że graf układa się inaczej niż w artykule

    std::string ant_type;
    switch (ant->type) {
        case Packet::Type::forward:
            ant_type = "forward";
            break;
        case Packet::Type::backward:
            ant_type = "backward";
            break;
        default:
            ant_type = "unexpected";
    }
    std::cout << endl << "### Passing " << ant_type << " ant. Previous address " << previousAddress << " current address " << address << std::endl;

    std::shared_ptr<RoutingEntry> entry = getEntryForDestinationAndHop(ant->sourceAddress, previousAddress);
    if (entry == NULL) {
        cout << "@@@ Pushing entry onto Node's routingTable\n";
        entry = std::make_shared<RoutingEntry>(RoutingEntry(ant->sourceAddress, previousAddress));
        routingTable.push_back(entry);
        entry->increasePheromone(); //TODO wartość feromonów jest obliczana w zależności od liczby aktualnie wykonanych hopek (źródło: artykuł).
    }
    

    if(std::get<1>(visitedAnts.insert(ant->sequenceNumber))) { // if ant did not visit before

        if (ant->destinationAddress == address) {
            switch (ant->type) {
                case Packet::Type::forward: {
                    cout<< "\n### Forward Ant travelled to destination.\n";
                    break;
                }
                case Packet::Type::backward:
                    cout<< "\n### Backward Ant travelled back to source.\n";
                    break;
                default:
                    throw std::runtime_error("Unexpected discovery ant type");
            }
        } 
        else {
            for (auto neighbour : neighbours) {
                if (neighbour->address != previousAddress) {
                    std::cout << "Current " << address << " neighbour " << neighbour->address << " " << __FUNCTION__ << std::endl;
                    neighbour->passDiscoveryAnt(address, ant);
                }
            }
        }
    } else {
        cout << "### Ignoring " << ant_type << " ant " << ant->sequenceNumber << " at Node with address " << address << endl;
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
