//
// Created by Tomasz Kubrak on 11/01/17.
//

#include "Graph.h"
#include "Packet.h"
#include <iostream>
#include <algorithm>
#include <memory>

using namespace std;

void Graph::addNode(std::shared_ptr<Node> node) {
    nodes.push_back(node);
}

void Graph::sendData(int senderAddress, int destinationAddress) {
    tPacketptr packet = std::make_shared<Packet>(senderAddress, destinationAddress);

    std::shared_ptr<Node> source, destination;
    std::for_each(nodes.begin(), nodes.end(), [&](std::shared_ptr<Node> node) {
        if (node->address == senderAddress) {
            source = node;
        }

        if (node->address == destinationAddress) {
            destination = node;
        }
    });

    if (source != NULL) {

    	cout<<"\n### Sending packet ...\n";

        //Forward search and backward search must happen sequentially
        tPacketptr forwardAnt = std::make_shared<Packet>(senderAddress, destinationAddress, Packet::Type::forward);
        source->sendPacket(1111, forwardAnt);
        for(int i=0; i < 20; ++i)
            tick();
        tPacketptr backwardAnt = std::make_shared<Packet>(destinationAddress, senderAddress, Packet::Type::backward);
        destination->sendPacket(8888, backwardAnt);
        for(int i=0; i < 20; ++i)
            tick();
        source->sendPacket(6666, packet); //TODO how to provide first address?
        for(int i=0; i < 20; ++i)
            tick();

        cout<<"\n### Showing address of graph's nodes and it's routing entries \n";
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

void Graph::tick() {

    cout<<"\n!!!!!!!!!!!!!!!!!!!!! TICK !!!!!!!!!!!!!!!!!!!!!!!!! \n";
    
    std::for_each(nodes.begin(), nodes.end(), [&](std::shared_ptr<Node> node) {
        node->tick();
    });
    std::for_each(nodes.begin(), nodes.end(), [&](std::shared_ptr<Node> node) {
        node->postTick();
    });
}
