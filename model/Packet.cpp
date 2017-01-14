//
// Created by Tomasz Kubrak on 11/01/17.
//

#include "Packet.h"
#include <cstdlib>

Packet::Packet(int src, int dest, Type type)
  : sourceAddress(src), destinationAddress(dest), 
    sequenceNumber(rand() % 1000000 +10000), type(type) {}
