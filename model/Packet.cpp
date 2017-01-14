//
// Created by Tomasz Kubrak on 11/01/17.
//

#include "Packet.h"
#include <cstdlib>

Packet::Packet(int src, int dest, Type type)
  : sourceAddress(src), destinationAddress(dest), 
    sequenceNumber(rand() % 1000000 +10000), type(type), hops_count(0)
{
    switch (type) {
        case Type::forward:
            type_string = "forward";
            break;
        case Type::backward:
            type_string = "backward";
            break;
        case Type::regular:
            type_string = "regular";
            break;
        case Type::route_error:
            type_string = "route_error";
            break;
        case Type::duplicate_error:
            type_string = "duplicate_error";
            break;
        default:
            throw std::runtime_error("Unexpected ant type");
    }
}
