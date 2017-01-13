#include "Node.hpp"
#include <memory>
#include <algorithm>
#include "Graph.h"

using namespace std;

static Graph graph;

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

bool Node::sendPacket(Packet packet) {

    bool afterDiscovery = false;
    tRoutingEntryVec entries = findDestinationEntries(packet);

    updatePherNodes();

    //we have to check if we have entries and if we are not in destination node - to avoid sending discovery
    if ((entries.size() == 0) && (address != packet.destinationAddress)) {
		startForwardAntPhase(packet.destinationAddress);
        //TODO : handle no route to destination case?
        afterDiscovery = true;
    } else {
        if(address != packet.destinationAddress){
       shared_ptr<RoutingEntry> bestPath = findBestPath(entries);
       shared_ptr<Node> bestNode = NULL;

       //find the best node by address
       for(auto node : neighbours){
           if(node->address == bestPath->nextHopAddress){
               bestNode = node;
               break;
           }
       }

       cout<< "\n### Packet in node @address: " << address<< "\n Now sending packet to Node @address :" << bestNode->address << endl;
        bestNode->sendPacket(packet);
        bestPath->increasePheromone();
    }
        else cout<<"\n### Packet reached destination!!!\n";
    }
    return afterDiscovery;
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

tRoutingEntryVec Node::findDestinationEntries(Packet packet) {
    tRoutingEntryVec entries;

    for_each(routingTable.begin(), routingTable.end(), [&](std::shared_ptr<RoutingEntry> entry) {
        if (entry->destinationAddress == packet.destinationAddress) {
            entries.push_back(entry);
        }
    });

    return entries;
}

void Node::startForwardAntPhase(int destinationAddress) {
    Packet forwardAnt = Packet(address, destinationAddress);

    for (auto neighbour : neighbours) {
        neighbour->passForwardAnt(address, forwardAnt);
    }
}

void Node::passForwardAnt(int previousAddress, Packet ant) {

    std::cout << endl << "### Passing Forward Ant. Previous address " << previousAddress << " current address " << address << std::endl;

    if (ant.destinationAddress == address) {
        cout<< "\n### Sending Backward Ant \n";
        startBackwardAntPhase(ant);

    } else {

        auto it = std::find(visitedForwardAnts.begin(), visitedForwardAnts.end(), ant.sequenceNumber);
        if(it == visitedForwardAnts.end()) {

            visitedForwardAnts.push_back(ant.sequenceNumber);

            std::shared_ptr<RoutingEntry> entry = getEntryForDestinationAndHop(ant.sourceAddress, previousAddress);

            if (entry == NULL) {
                cout << "@@@ Pushing entry onto Node's routingTable\n";
                entry = std::make_shared<RoutingEntry>(RoutingEntry(ant.sourceAddress, previousAddress));
                routingTable.push_back(entry);
            }

            entry->increasePheromone();

            for (auto neighbour : neighbours) {

                std::cout << "Current " << address << " neighbour " << neighbour->address << " " << __FUNCTION__ << std::endl;

                if (neighbour->address != previousAddress) {
                    neighbour->passForwardAnt(address, ant);
                }
            }

        } else {
            cout << "### Deleted forward ant " << ant.sequenceNumber << " at Node " << name << endl;
        }
    }

}

void Node::startBackwardAntPhase(Packet packet) {
    std::cout << "### Backward Ant Sent" << std::endl;

    Packet backwardAnt = Packet(address, packet.sourceAddress);
    auto it = std::find(visitedBackwardAnts.begin(), visitedBackwardAnts.end(), packet.sequenceNumber);

    if(it == visitedBackwardAnts.end()) {
        visitedBackwardAnts.push_back(packet.sequenceNumber);

        for (auto neighbour : neighbours) {
            neighbour->passBackwardAnt(address, backwardAnt);
        }

    }
}

void Node::passBackwardAnt(int previousAddress, Packet ant) {

    std::cout << std::endl << "### Passing Backward Ant. Previous address " << previousAddress << " current address " << address << std::endl;

    if (ant.destinationAddress == address) {

        // Start transmission

    } else {

        auto it = std::find(visitedBackwardAnts.begin(), visitedBackwardAnts.end(), ant.sequenceNumber);
        if(it == visitedBackwardAnts.end()) {

            visitedBackwardAnts.push_back(ant.sequenceNumber);

            std::shared_ptr<RoutingEntry> entry = getEntryForDestinationAndHop(ant.sourceAddress, previousAddress);

            if (entry == NULL) {
                entry = std::make_shared<RoutingEntry>(RoutingEntry(ant.sourceAddress, previousAddress));
                routingTable.push_back(entry);
            }
            entry->increasePheromone();

            for (auto neighbour : neighbours) {

                    std::cout << "Current " << address << " neighbour " << neighbour->address << " " << __FUNCTION__ << std::endl;

                    if (neighbour->address != previousAddress) {
                        neighbour->passBackwardAnt(address, ant);
                    }
            }
        } else {
            cout << "### Deleted backward ant " << ant.sequenceNumber << " at Node " << name << endl;
        }
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
