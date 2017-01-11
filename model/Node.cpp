#include "Node.hpp"
#include <memory>

// Constructors

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
    return routingTable;
}

void Node::startForwardAntPhase(int destinationAddress) {
    Packet forwardAnt = Packet(address, destinationAddress);

    for (auto neighbour : neighbours) {
        neighbour->passForwardAnt(address, forwardAnt);
    }
}

void Node::passForwardAnt(int previousAddress, Packet ant) {
    std::shared_ptr<RoutingEntry> entry = getEntryForDestinationAndHop(ant.sourceAddress, previousAddress);
    if (entry == NULL) {
        entry = std::make_shared<RoutingEntry>(RoutingEntry(ant.sourceAddress, previousAddress));
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
