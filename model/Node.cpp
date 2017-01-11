#include "Node.hpp"
#include <memory>

void Node::addNeighbour(std::shared_ptr <Node> node) {

    if(std::find(neighbours.begin(), neighbours.end(), node) != neighbours.end() == false) {
        neighbours.push_back(node);
        std::shared_ptr<Node> self(this);
        node->neighbours.push_back(self);
    }

}

void Node::forwardAntPhase(int destinationAddress) {

}

Node::Node(std::string name, int address) : name(name), address(address) {}