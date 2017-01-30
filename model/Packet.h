//
// Created by Tomasz Kubrak on 11/01/17.
//

#ifndef MANET_ROUTING_ACO_PACKET_H
#define MANET_ROUTING_ACO_PACKET_H


class Packet {

public:
    enum class Type {regular, forward, backward, route_error, duplicate_error, invalid};

    __device__
    Packet()
    : sourceAddress(0),
      destinationAddress(0),
      sequenceNumber(0),
      type(Type::invalid),
      next_hop(0),
      hops_count(0)
    {}

    __device__
    Packet(int src, int dst, int sequenceNumber, Type type = Type::regular) 
    : sourceAddress(src), 
      destinationAddress(dst),
      sequenceNumber(sequenceNumber), 
      type(type), 
      next_hop(0),
      hops_count(0)
    {}
    // Hiding copy constructor helps discovering unplanned generation of new sequence number.
    // Packet(const Packet&) = default;
    // Packet& operator=(const Packet&) = default;

    /*const*/ Type type;
    /*const*/ int sequenceNumber;
    int sourceAddress;
    int destinationAddress;
    int next_hop;
    unsigned hops_count;
};

#endif //MANET_ROUTING_ACO_PACKET_H
