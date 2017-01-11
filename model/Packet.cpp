//
// Created by Tomasz Kubrak on 11/01/17.
//

#include "Packet.h"
#include <time.h>
#include <cstdlib>

Packet::Packet(int src, int dest) : sourceAddress(src), destinationAddress(dest) {
    srand(time(NULL));
    sequenceNumber = rand();
}