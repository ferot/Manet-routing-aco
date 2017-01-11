#include "RoutingEntry.hpp"

RoutingEntry::RoutingEntry(int dest, int nhop) : destinationAddress(dest),
                                                                                     nextHopAddress(nhop),
                                                                                     pheromone(0.0) {

}
