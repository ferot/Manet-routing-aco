#pragma once

constexpr float PHEROMONE_DELTA = 0.1;
constexpr float EVAPORATION_FACTOR = 0.01;

class RoutingEntry {

public:
    float pheromone;

    RoutingEntry() : pheromone(1.0f) 
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
