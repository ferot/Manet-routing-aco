/*
 * main.cpp
 *
 *  Created on: Jan 11, 2017
 *      Author: fero
 */

#include "model/Graph.h"

Graph mockSimpleGraph();


int main(){

    Graph graph = mockSimpleGraph();

    graph.sendData(2, 3);

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

    node1.addNeighbour(std::make_shared<Node>(node2));
    node1.addNeighbour(std::make_shared<Node>(node3));

    node3.addNeighbour(std::make_shared<Node>(node4));

    node4.addNeighbour(std::make_shared<Node>(node5));
    node4.addNeighbour(std::make_shared<Node>(node7));

    node5.addNeighbour(std::make_shared<Node>(node6));

    node6.addNeighbour(std::make_shared<Node>(node8));

    node7.addNeighbour(std::make_shared<Node>(node8));

    graph.addNode(node1);
    graph.addNode(node2);
    graph.addNode(node3);
    graph.addNode(node4);
    graph.addNode(node5);
    graph.addNode(node6);
    graph.addNode(node7);
    graph.addNode(node8);

    return graph;
}
