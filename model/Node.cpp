#include "Node.hpp"
#include <memory>

// Constructors

using namespace std;

Node::Node(std::string name, int address) : name(name), address(address) {}

// Public

void Node::addNeighbour(std::shared_ptr <Node> node) {

    if(std::find(neighbours.begin(), neighbours.end(), node) != neighbours.end() == false) {
        neighbours.push_back(node);
    }

}



void Node::sendPacket(Packet packet) {
    std::vector<std::shared_ptr<RoutingEntry> > entries = findDestinationEntries(packet);
    if (entries.size() == 0) {

        startForwardAntPhase(packet.destinationAddress);

    } else {
        // Send data
    }
}

// Private

std::vector<std::shared_ptr<RoutingEntry> > Node::findDestinationEntries(Packet packet) {
    std::vector<std::shared_ptr<RoutingEntry> > entries;
    std::for_each(routingTable.begin(), routingTable.end(), [&](std::shared_ptr<RoutingEntry> entry) {
        if (entry->destinationAddress == packet.destinationAddress) {
            entries.push_back(entry);
        }
    });

    return entries;
}

void Node::startForwardAntPhase(int destinationAddress) {
    Packet forwardAnt = Packet(address, destinationAddress);

    for (auto neighbour : neighbours) {

        cout << "neighbours " << neighbour->neighbours.size() << endl;

        neighbour->passForwardAnt(address, forwardAnt);
    }
}

void Node::passForwardAnt(int previousAddress, Packet ant) {

    std::cout << "prev " << previousAddress << " current " << address << std::endl;

    if (ant.destinationAddress == address) {

        startBackwardAntPhase(ant.sourceAddress);

    } else {

        if(std::find(visitedForwardAnts.begin(), visitedForwardAnts.end(), ant.sequenceNumber) != visitedForwardAnts.end() == false) {

//            cout << "nejbory " << neighbours.size() << endl;

            visitedForwardAnts.push_back(ant.sequenceNumber);

            std::shared_ptr<RoutingEntry> entry = getEntryForDestinationAndHop(ant.sourceAddress, previousAddress);

            if (entry == NULL) {
                entry = std::make_shared<RoutingEntry>(RoutingEntry(ant.sourceAddress, previousAddress));
                routingTable.push_back(entry);
            }

            entry->increasePheromone();

            for (auto neighbour : neighbours) {

                std::cout << "Current " << address << " neighbour " << neighbour->address << std::endl;

                if (neighbour->address != previousAddress) {
                    neighbour->passForwardAnt(address, ant);
                }
            }
        }
    }

}

void Node::startBackwardAntPhase(int sourceAddress) {
    std::cout << "back" << std::endl;
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
