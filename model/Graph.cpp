//
// Created by Tomasz Kubrak on 11/01/17.
//

#include "Graph.h"
#include "Packet.h"
#include <iostream>

void Graph::addNode(Node node) {
    nodes.push_back(std::make_shared<Node>(node));
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