#include "RoutingEntry.hpp"
#include "Graph.h"

RoutingEntry::RoutingEntry(int dest, int nhop) : destinationAddress(dest),
                                                                                     nextHopAddress(nhop),
                                                                                     pheromone(0.0) {

}

void RoutingEntry::increasePheromone() {
    pheromone += PHEROMONE_DELTA;
}