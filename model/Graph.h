//
// Created by Tomasz Kubrak on 11/01/17.
//

#ifndef MANET_ROUTING_ACO_GRAPH_H
#define MANET_ROUTING_ACO_GRAPH_H

#include "RoutingEntry.hpp"
#include <vector>
#include <memory>

const float PHEROMONE_DELTA = 0.05;
const float EVAPORATION_FACTOR = 0.25;
static void updatePherNodes();

class Node;

class Graph {


    public:
    std::vector<std::shared_ptr<Node> > nodes;
    void addNode(std::shared_ptr<Node> node);
    void sendData(int senderAddress, int destinationAddress);
};

#endif //MANET_ROUTING_ACO_GRAPH_H
