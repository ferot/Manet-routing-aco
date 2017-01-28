/*
 * main.cpp
 *
 *  Created on: Jan 11, 2017
 *      Author: fero
 */
#include <time.h>
#include <cstdlib>
#include <fstream>
#include <string>
#include "model/Graph.h"
#include "model/json.hpp"

using namespace std;
using json = nlohmann::json;

Graph mockSimpleGraph();
Graph loadGraph(std::string filename);

typedef std::shared_ptr<Node> tNodeptr;

int main(){
    srand(time(NULL)); //it has global effect

    Graph graph = loadGraph("graph.json");

    int testSource = 21;
    int testDestination = 10;

    for (int k=1; k < 100; ++k){
//        graph.sendData(rand() % graph.nodes.size(), rand() % graph.nodes.size());
        graph.sendData(testSource, testDestination);
        for(int i=0; i < 10 + rand() % 20; ++i) graph.tick();
    }

    for(int i=0; i < 20; ++i)
        graph.tick();

    graph.printRoutingTables();

    int shortestPath = graph.getShortestPath(testSource, testDestination);
    cout << "Shortest path " << shortestPath << endl;

	return 0;
}

Graph mockSimpleGraph() {
	Graph graph;

    tNodeptr n1Ptr = std::make_shared<Node>("S", 1);
    tNodeptr n2Ptr = std::make_shared<Node>("1", 2);
    tNodeptr n3Ptr = std::make_shared<Node>("2", 3);
    tNodeptr n4Ptr = std::make_shared<Node>("3", 4);
    tNodeptr n5Ptr = std::make_shared<Node>("4", 5);
    tNodeptr n6Ptr = std::make_shared<Node>("5", 6);
    tNodeptr n7Ptr = std::make_shared<Node>("6", 7);
    tNodeptr n8Ptr = std::make_shared<Node>("D", 8);

    n1Ptr->addNeighbour(n2Ptr);
    n2Ptr->addNeighbour(n1Ptr);

    n1Ptr->addNeighbour(n3Ptr);
    n3Ptr->addNeighbour(n1Ptr);

    n3Ptr->addNeighbour(n4Ptr);
    n4Ptr->addNeighbour(n3Ptr);

    n4Ptr->addNeighbour(n5Ptr);
    n4Ptr->addNeighbour(n7Ptr);
    n5Ptr->addNeighbour(n4Ptr);
    n7Ptr->addNeighbour(n4Ptr);

    n5Ptr->addNeighbour(n6Ptr);
    n6Ptr->addNeighbour(n5Ptr);

    n6Ptr->addNeighbour(n8Ptr);
    n8Ptr->addNeighbour(n6Ptr);

    n7Ptr->addNeighbour(n8Ptr);
    n8Ptr->addNeighbour(n7Ptr);

    graph.addNode(n1Ptr);
    graph.addNode(n2Ptr);
    graph.addNode(n3Ptr);
    graph.addNode(n4Ptr);
    graph.addNode(n5Ptr);
    graph.addNode(n6Ptr);
    graph.addNode(n7Ptr);
    graph.addNode(n8Ptr);

    return graph;
}


Graph loadGraph(std::string filename) {
    std::ifstream inputStream(filename);
    json graphJSON;
    inputStream >> graphJSON;

    Graph graph;

    for (auto jsonNode : graphJSON["nodes"]) {
        int nodeNumber = jsonNode[0];
        std::string nodeName = std::to_string(nodeNumber);
        tNodeptr node = std::make_shared<Node>(nodeName, nodeNumber);
        graph.addNode(node);
    }

    for (auto jsonEdge : graphJSON["edges"]) {
        int fromNodeNumber = jsonEdge[0];
        int toNodeNumber = jsonEdge[1];

        tNodeptr fromNode = graph.nodes[fromNodeNumber];
        tNodeptr toNode = graph.nodes[toNodeNumber];

        fromNode->addNeighbour(toNode);
        toNode->addNeighbour(fromNode);
    }

    return graph;
}
