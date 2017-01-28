//
// Created by Tomasz Kubrak on 11/01/17.
//

#ifndef MANET_ROUTING_ACO_PACKET_H
#define MANET_ROUTING_ACO_PACKET_H


class Packet {

public:
    enum class Type {regular, forward, backward, route_error, duplicate_error, invalid};

    Packet()
    : sourceAddress(0),
      sequenceNumber(0),
      type(Type::invalid),
      hops_count(0)
    {}

    Packet(int src, int sequenceNumber, Type type = Type::regular) 
    : sourceAddress(src), 
      sequenceNumber(sequenceNumber), 
      type(type), 
      hops_count(0)
    {}
    // Hiding copy constructor helps discovering unplanned generation of new sequence number.
    // Packet(const Packet&) = default;
    // Packet& operator=(const Packet&) = default;

    /*const*/ Type type;
    /*const*/ int sequenceNumber;
    int sourceAddress;
    unsigned hops_count;
};

#endif //MANET_ROUTING_ACO_PACKET_H
