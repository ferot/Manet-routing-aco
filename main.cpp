/*
 * main.cpp
 *
 *  Created on: Jan 11, 2017
 *      Author: fero
 */

#include "model/Graph.h"

using namespace std;

Graph mockSimpleGraph();

typedef std::shared_ptr<Node> tNodeptr;

static Graph graph;

int main(){

    graph = mockSimpleGraph();

    graph.sendData(1, 8);

	return 0;
}

Graph mockSimpleGraph() {
	Graph graph;

	Node node1("S", 1);
	Node node2("1", 2);
	Node node3("2", 3);
	Node node4("3", 4);
	Node node5("4", 5);
	Node node6("5", 6);
	Node node7("6", 7);
	Node node8("D", 8);

    tNodeptr n1Ptr = std::make_shared<Node>(node1);
    tNodeptr n2Ptr = std::make_shared<Node>(node2);
    tNodeptr n3Ptr = std::make_shared<Node>(node3);
    tNodeptr n4Ptr = std::make_shared<Node>(node4);
    tNodeptr n5Ptr = std::make_shared<Node>(node5);
    tNodeptr n6Ptr = std::make_shared<Node>(node6);
    tNodeptr n7Ptr = std::make_shared<Node>(node7);
    tNodeptr n8Ptr = std::make_shared<Node>(node8);

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
