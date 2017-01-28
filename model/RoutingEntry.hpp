#pragma once

#include <cmath>

constexpr float PHEROMONE_DELTA = 0.1;
constexpr float EVAPORATION_FACTOR = 0.01;

class RoutingEntry {

public:
    float pheromone;

    RoutingEntry() : pheromone(-1.0f) 
    {}

    void increasePheromone(unsigned num_hops)
    {
        pheromone += PHEROMONE_DELTA * std::pow(0.9, num_hops);
    }

    void evaporatePheromone()
    {
        pheromone = (1-EVAPORATION_FACTOR)*pheromone;
    }

};
