#include "RoutingEntry.hpp"

RoutingEntry::RoutingEntry(std::shared_ptr<Node> dest, std::shared_ptr<Node> nhop) : destination(dest),
                                                                                     next_hop(nhop),
                                                                                     pheromone(0.0) {

}
