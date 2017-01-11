//
// Created by Tomasz Kubrak on 11/01/17.
//

#include "Graph.h"
#include "Packet.h"
#include <iostream>
#include <algorithm>

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

        for (auto node : nodes) {
            cout << node->address << " -----" << endl;

            for (auto entry : node->routingTable) {
                cout << "Destination " << entry->destinationAddress << " next hop " << entry->nextHopAddress << " pheromone " << entry->pheromone << endl;
            }

        }

    } else {
        std::cout << "No such node" << std::endl;
    }
}
