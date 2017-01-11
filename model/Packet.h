//
// Created by Tomasz Kubrak on 11/01/17.
//

#ifndef MANET_ROUTING_ACO_PACKET_H
#define MANET_ROUTING_ACO_PACKET_H


class Packet {

    public:

        Packet(int src, int dest);

    int sequenceNumber;
    int destinationAddress;
    int sourceAddress;

};


#endif //MANET_ROUTING_ACO_PACKET_H
