//
// Created by Tomasz Kubrak on 11/01/17.
//

#include "Graph.h"
#include "Packet.h"
#include <iostream>

using namespace std;

void Graph::addNode(std::shared_ptr<Node> node) {
    nodes.push_back(node);
}

void Graph::sendData(int senderAddress, int destinationAddress) {
    Packet packet(senderAddress, destinationAddress);

    std::shared_ptr<Node> source;
        std::for_each(nodes.begin(), nodes.end(), [&](std::shared_ptr<Node> node) {
            if (node->address == senderAddress) {
                source = node;
            }
        });

    if (source != NULL) {

        source->sendPacket(packet);
    } else {
        std::cout << "No such node" << std::endl;
    }
}