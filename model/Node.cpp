#include "Node.hpp"
#include <memory>
#include <algorithm>

// Constructors

using namespace std;

typedef std::vector<std::shared_ptr<RoutingEntry> > routingEntryVec;

Node::Node(std::string name, int address) : name(name), address(address) {}

// Public

void Node::addNeighbour(std::shared_ptr <Node> node) {

    auto it =find(neighbours.begin(), neighbours.end(), node);

	if(it == neighbours.end()) {
        neighbours.push_back(node);
    }
}



void Node::sendPacket(Packet packet) {

	routingEntryVec entries = findDestinationEntries(packet);

	if (entries.size() == 0) {
		startForwardAntPhase(packet.destinationAddress);
    } else {
        // Send data
    }
}

// Private

routingEntryVec Node::findDestinationEntries(Packet packet) {
    routingEntryVec entries;

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

        startBackwardAntPhase(ant);

    } else {

        auto it = std::find(visitedForwardAnts.begin(), visitedForwardAnts.end(), ant.sequenceNumber);
        if(it == visitedForwardAnts.end()) {

            visitedForwardAnts.push_back(ant.sequenceNumber);

            std::shared_ptr<RoutingEntry> entry = getEntryForDestinationAndHop(ant.sourceAddress, previousAddress);

            if (entry == NULL) {
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
    std::shared_ptr<RoutingEntry> entry;
    std::for_each(routingTable.begin(), routingTable.end(), [&](std::shared_ptr<RoutingEntry> routingEntry) {
        if (routingEntry->destinationAddress == dest && routingEntry->nextHopAddress == hop) {
            entry = routingEntry;
        }
    });

    return entry;
}
