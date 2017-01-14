//
// Created by Tomasz Kubrak on 11/01/17.
//

#ifndef MANET_ROUTING_ACO_PACKET_H
#define MANET_ROUTING_ACO_PACKET_H

#include <memory>

class Packet;
typedef std::shared_ptr<Packet> tPacketptr;

class Packet {

    public:
enum class Type {regular, forward, backward};

    Packet(int src, int dest, Type type = Type::regular);

    const Type type;
    const int sequenceNumber;
    int destinationAddress;
    int sourceAddress;

    private:
    // Hiding copy constructor helps discovering unplanned generation of new sequence number.
    Packet(const Packet& that); 

};

#endif //MANET_ROUTING_ACO_PACKET_H
