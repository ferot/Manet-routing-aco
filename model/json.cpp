#include "json.hpp"
#include "config.h"
#include <fstream>

#define UNCUDA
#include "RoutingEntry.hpp"

void initializeRoutingTable(std::string file_name, RoutingEntry (&routing_table)[nodes_num][nodes_num][nodes_num])
{
    std::ifstream inputStream("graph.json");
    nlohmann::json graphJSON;
    inputStream >> graphJSON;

    for (auto jsonEdge : graphJSON["edges"]) {
        int fromNodeNumber = jsonEdge[0];
        int toNodeNumber = jsonEdge[1];

        for (int i=0; i < nodes_num; ++i)
        {
            routing_table[fromNodeNumber][i][toNodeNumber].pheromone = 1.0;
            routing_table[toNodeNumber][i][fromNodeNumber].pheromone = 1.0;
        }
    }
}