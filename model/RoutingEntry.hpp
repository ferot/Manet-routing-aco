#pragma once

#include <cmath>

constexpr float PHEROMONE_DELTA = 0.01;
constexpr float EVAPORATION_FACTOR = 0.001;

class RoutingEntry {

public:
    float pheromone;

    RoutingEntry() : pheromone(-1.0f) 
    {}

    static float calculateIncreasement(unsigned num_hops)
    {
        return PHEROMONE_DELTA * std::pow(0.9, num_hops);
    }

    void increasePheromone(float increase_pheromone)
    {
        pheromone += increase_pheromone;
    }

    void evaporatePheromone()
    {
        pheromone = (1-EVAPORATION_FACTOR)*pheromone;
    }

};
