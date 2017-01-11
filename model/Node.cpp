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
    std::vector<RoutingEntry> entries = findDestinationEntries(packet);
    if (entries.size() == 0) {
        forwardAntPhase(packet.destinationAddress);
    } else {
        // Send data
    }
}

// Private

std::vector<RoutingEntry> Node::findDestinationEntries(Packet packet) {
    return routing_table;
}

void Node::forwardAntPhase(int destinationAddress) {

}