#include "RoutingEntry.hpp"

RoutingEntry::RoutingEntry (std::unique_ptr<Node> dest, std::unique_ptr<Node> nhop ) :
pheromone(0.0),destination(dest), next_hop(nhop)
{
}
