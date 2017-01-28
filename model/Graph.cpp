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

    std::shared_ptr<Node> source;
    std::for_each(nodes.begin(), nodes.end(), [&](std::shared_ptr<Node> node) {
        if (node->address == senderAddress) {
            source = node;
        }
    });

    if (source != NULL) {
    	cout<<"\n### Sending packet ...\n";
        source->sendPacket(6666, packet); //TODO how to provide first address?
    } else {
        std::cout << "No such node" << std::endl;
    }
}

void Graph::tick() {

    // cout<<"\n!!!!!!!!!!!!!!!!!!!!! TICK !!!!!!!!!!!!!!!!!!!!!!!!! \n";
    
    std::for_each(nodes.begin(), nodes.end(), [&](std::shared_ptr<Node> node) {
        node->tick();
    });
    std::for_each(nodes.begin(), nodes.end(), [&](std::shared_ptr<Node> node) {
        node->postTick();
    });
}

void Graph::printRoutingTables() {
    cout<<"\n### Showing address of graph's nodes and it's routing entries \n";
    for (auto node : nodes) {
        cout << node->address << " -----" << endl;

        sort(node->routingTable.begin(), node->routingTable.end(),
             [](const std::shared_ptr<RoutingEntry> & a, const std::shared_ptr<RoutingEntry> & b) -> bool {
                 if (a->destinationAddress == b->destinationAddress) {
                     return a->nextHopAddress < b->nextHopAddress;
                 } else {
                     return a->destinationAddress < b->destinationAddress;
                 }
             });

        for (auto entry : node->routingTable) {
            cout << "Destination " << entry->destinationAddress << " next hop " << entry->nextHopAddress << " pheromone " << entry->pheromone << endl;
        }

    }
}

int Graph::getShortestPath(int sourceAddress, int destinationAddress) {
    std::shared_ptr<Node> nextNode = getNodeForAddress(sourceAddress);
    int hopsCount = 0;
    vector<int> visitedNodes;

     do {
        int nextNodeAddress = nextNode->getDeterministicNextHop(sourceAddress, destinationAddress);

        if(find(visitedNodes.begin(), visitedNodes.end(), nextNodeAddress) != visitedNodes.end()) {
            std::cout << "Loop" << std::endl;
            return -1;
        }

        visitedNodes.push_back(nextNodeAddress);

        cout << nextNodeAddress << endl;
        nextNode = getNodeForAddress(nextNodeAddress);
        if (nextNode != NULL) {
            hopsCount++;
        } else {
            std::cout << "No next hop?" << std::endl;
            return -1;
        }
    } while(nextNode->address != destinationAddress);

    return hopsCount;

}

std::shared_ptr<Node> Graph::getNodeForAddress(int address) {

    for (auto node : nodes) {
        if (node->address == address) {
            return node;
        }
    }

    return NULL;
}