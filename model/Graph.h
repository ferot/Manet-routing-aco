//
// Created by Tomasz Kubrak on 11/01/17.
//

#ifndef MANET_ROUTING_ACO_GRAPH_H
#define MANET_ROUTING_ACO_GRAPH_H

#include "RoutingEntry.hpp"
#include <vector>

class Graph {

    std::vector<std::shared_ptr<Node> > nodes;

    public:
        void addNode(Node node);
        void sendData(int senderAddress, int destinationAddress);
};

#endif //MANET_ROUTING_ACO_GRAPH_H
