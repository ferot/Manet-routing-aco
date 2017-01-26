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
    enum class Type {regular, forward, backward, route_error, duplicate_error};

    Packet(int src, int sequenceNumber, Type type = Type::regular);
    // Hiding copy constructor helps discovering unplanned generation of new sequence number.
    Packet(const Packet&) = delete;
    Packet& operator=(const Packet&) = delete;

    const Type type;
    const int sequenceNumber;
    // int destinationAddress;
    int sourceAddress;
    // unsigned last_hop;
    unsigned hops_count;
    // std::string type_string;
};

#endif //MANET_ROUTING_ACO_PACKET_H
