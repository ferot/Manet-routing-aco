#pragma once
#include <vector>
#include <memory>
#include "Node.hpp"

constexpr float PHEROMONE_DELTA = 0.1;
constexpr float EVAPORATION_FACTOR = 0.005;

class RoutingEntry {

public:
    float pheromone;

    RoutingEntry() : pheromone(0.0f) 
    {}

    void increasePheromone()
    {
        pheromone += PHEROMONE_DELTA;
    }

    void evaporatePheromone()
    {
        pheromone = (1-EVAPORATION_FACTOR)*pheromone;
    }

};
