// #include <curand.h>
// #include <curand_kernel.h>

#include <cstdlib>
#include <ctime>
#include <cmath>
#include <cstring>
#include <memory>
#include <iostream>

#include "Packet.h"
#include "RoutingEntry.hpp"

constexpr int MAX = 100000;
constexpr int MAX_BUFFER_SIZE = 10;
constexpr int nodes_num = 10;
constexpr long total_elem_num = nodes_num * nodes_num * nodes_num;

class PacketBuffer
{
public:
    PacketBuffer() : num_of_elements_(0) {}
    int num_of_elements_;
    Packet buffer_[MAX_BUFFER_SIZE];

    bool addPacket(Packet& packet)
    {
        if (num_of_elements_ == MAX_BUFFER_SIZE)
        {
            return false;
        }
        else
        {
            buffer_[num_of_elements_] = packet;
            num_of_elements_ += 1;
            return true;
        }
    }
};

/*__device__*/ 
int calcElem(int node, int target, int neighbour)
{
    return (node*nodes_num + target)*nodes_num  + neighbour;
}


/*__device__*/ 
int drawNextHop(RoutingEntry* routing_for_neighbours, int node, int target)
{
    float pheromoneSum = 0.0;

    //count sum of pheromones for available routes
    for (int i=0; i < nodes_num; ++i)
    {
        int elem = calcElem(node, target, i);
        float pheromone = routing_for_neighbours[elem].pheromone;
        if (pheromone > 0.0)
        {
            pheromoneSum += pheromone;
        }
    }

    // curandState_t state; //TODO initialize this only once
    // curand_init(pheromoneSum, 0, 0, &state);
    // result = curand(&state) % MAX;
    float rand_num = std::rand();
    float part = rand_num / (float)RAND_MAX;
    float random = part * pheromoneSum;

    int choosenHop = -1;
    while(random > 0.0) {
        ++choosenHop;
        int elem = calcElem(node, target, choosenHop);
        float pheromone = routing_for_neighbours[elem].pheromone;
        if (pheromone > 0.0) // if is my neighbour
        {
            random -=  pheromone;
        }
    }

    return choosenHop;
}

struct PseudoCuda {
    int x;
};


// TO się musi wykonać sekwencyjnie. Czasu nie oszukasz. Ale może obejdzie się bez kopiowania do hosta
// [node][target][neighbour]

/*__global__*/ 
void nodesTick(PacketBuffer* incomming_buffers, 
               PacketBuffer* outgoing_buffers, 
               RoutingEntry* routing_table,
               int thread_num, int block_num, int thread_dim, int block_dim)
{
    PseudoCuda blockIdx, blockDim, threadIdx, threadDim;
    blockIdx.x = block_num;
    blockDim.x = block_dim;
    threadIdx.x = thread_num;
    threadDim.x = thread_dim;


    const int node = blockIdx.x;
    for (int target = threadIdx.x; target < blockDim.x; target += threadDim.x)
    {
        // updating routing table.
        for (int neighbour = 0; neighbour < blockDim.x; ++neighbour)
        {
            RoutingEntry& routing_entry = routing_table[calcElem(node, target, neighbour)];
            if (routing_entry.pheromone > 0.0)
            {
                routing_entry.evaporatePheromone();
            }
            outgoing_buffers[calcElem(neighbour, target, node)].num_of_elements_ = 0;
        }

        for (int prev_hop = 0; prev_hop < blockDim.x; ++prev_hop) //neighbours from which we reived packets
        {
            // if (routing_table[calcElem(node, target, prev_hop)].pheromone < 0.0)
            //     continue; //Not a neighbour


            PacketBuffer& current_incoming_packet_buffer = incomming_buffers[calcElem(node, target, prev_hop)];

            for (int i=0; i< current_incoming_packet_buffer.num_of_elements_; ++i)
            {
                Packet& current_packet = current_incoming_packet_buffer.buffer_[i];
                // TODO just now only regular packets
                // processPacket(node, current_packet); //TODO where to copy (send)? Potentially many next hops.

                //Increase pheromone for the previous hop
                routing_table[calcElem(node, current_packet.sourceAddress, prev_hop)].increasePheromone(current_packet.hops_count);

                if (node == target)
                {
                    std::cout << "Packet " << current_packet.sequenceNumber << " reached node " << node << " after " << current_packet.hops_count << " hops." << std::endl;
                }
                else
                {
                    // TODO temporarly here
                    int next_hop = drawNextHop(routing_table, node, target);
                    current_packet.hops_count += 1;

                    // sending packet
                    PacketBuffer& outgoing_packet_buffer = outgoing_buffers[calcElem(next_hop, target, node)];
                    bool added = outgoing_packet_buffer.addPacket(current_packet);
                    if (!added)
                    {
                        std::cout << "At node " << node << " packet " << current_packet.sequenceNumber << " that goes to " << next_hop << " was dropped due to exceeded buffer." << std::endl;
                    }

                    routing_table[calcElem(node, target, next_hop)].increasePheromone(current_packet.hops_count);

                    std::cout << "At node " << node << " packet " << current_packet.sequenceNumber << " whose target is " << target << " came from " << prev_hop << " and goes to " << next_hop << "." << std::endl;
                }

            }
            
            //free incoming buffer
            current_incoming_packet_buffer.num_of_elements_ = 0;
        }
    }
}





// __device__ void processPacket(int fromNode, Packet& packet) 
// {
//     switch (packet.type) {
//         case Packet::Type::regular:
//             bool success = passRegularPacket(fromNode, packet);
//             // if (!success) {
//             //     tPacketptr forwardAnt = std::make_shared<Packet>(address, packet->destinationAddress, Packet::Type::forward);
//             //     sendPacket(address, forwardAnt);
//             // }
//             break;
//         // case Packet::Type::forward:
//         // case Packet::Type::backward:
//         //     passDiscoveryAnt(fromNode, packet);
//         //     break;
//         // case Packet::Type::route_error:
//         //     break;
//         // case Packet::Type::duplicate_error:
//         //     passLoopPacket(packet);
//         //     break;
//         default:
//             //rather log and ignore it
//             throw std::runtime_error("Unexpected ant type");
//     }

//     packet.sourceAddress = fromNode;
//     packet.hops_count += 1;
// }




// __device__ bool passRegularPacket(int previousAddress, Packet& packet)
// {
//     //we have to check if we have entries and if we are not in destination node - to avoid sending discovery
//     if (address != packet->destinationAddress) {
//         tRoutingEntryVec entries = findDestinationEntries(packet);

//         if (entries.empty()) {
//             // cout<<"\n### Don't know where to send packet!!!\n";
//             return false;
//         } 
//         else {
//             shared_ptr<RoutingEntry> bestPath = drawNextHop(entries);
//             shared_ptr<Node> bestNode = NULL;

//             //find the best node by address
//             for(auto node : neighbours){
//                 if(node->address == bestPath->nextHopAddress){
//                     bestNode = node;
//                     break;
//                 }
//             }

//             //TODO jeszcze trzeba zwiększyć feromony krawędzi, z której paczka przyszła
//             bestPath->increasePheromone();

//             // cout<< "\n### Packet in node @address: " << address<< "\n Now sending packet to Node @address :" << bestNode->address << endl;
//             bestNode->sendPacket(address, packet);
//         }
//     }
//     else {
//         // cout<<"\n### Packet reached destination!!!\n";
//     }

//     return true;
// }

// __device__ void passDiscoveryAnt(int previousAddress, Packet& packet) {

//     // std::cout << endl << "### Passing " << ant->type_string << " ant. Previous address " << previousAddress << " current address " << address << std::endl;

//     std::shared_ptr<RoutingEntry> entry = getEntryForDestinationAndHop(ant->sourceAddress, previousAddress);
//     if (entry == NULL && ant->sourceAddress != address) { //TODO czy wystarczająco zapobiega to powstawaniu pętli? W artykule jest trochę inaczej
//         // cout << "@@@ Pushing entry onto Node's routingTable" << std::endl;
//         entry = std::make_shared<RoutingEntry>(RoutingEntry(ant->sourceAddress, previousAddress));
//         routingTable.push_back(entry);
//         entry->increasePheromone(); //TODO wartość feromonów jest obliczana w zależności od liczby aktualnie wykonanych hopek (źródło: artykuł).
//     }
    
//     if(std::get<1>(visitedAnts.insert(std::make_pair(ant->sequenceNumber, previousAddress)))) { // if ant did not visit before
//         if (ant->destinationAddress == address) {
//             // cout<< "### "<< ant->type_string <<" Ant finished its journey." << std::endl;
//             if (ant->type == Packet::Type::forward) {
//                 tPacketptr backwardAnt = std::make_shared<Packet>(address, ant->sourceAddress, Packet::Type::backward);
//                 sendPacket(address, backwardAnt);
//             }
//         } 
//         //else {
//             for (auto neighbour : neighbours) {
//                 if (neighbour->address != previousAddress) {
//                     // std::cout << "Current " << address << " neighbour " << neighbour->address << " " << __FUNCTION__ << std::endl;
//                     neighbour->sendPacket(address, ant);
//                 }
//             }
//         //}
//     } else {
//         // cout << "### Ignoring " << ant->type_string << " ant " << ant->sequenceNumber << " at Node with address " << address << endl;
//     }
// }

// __device__ void passLoopPacket(Packet& packet) {
//     if (packet->destinationAddress == address)
//     {
//         //Delete looping route

//         tRoutingEntryVec::iterator it;
//         for (it = routingTable.begin(); it != routingTable.end(); ++it) {
//             if ((*it)->destinationAddress == packet->destinationAddress && 
//                 (*it)->nextHopAddress == packet->sourceAddress) 
//             {
//                 break;
//             }
//         }

//         if (it == routingTable.end())
//         {
//             throw std::runtime_error("Routing entry to delete was not found.");
//         }

//         routingTable.erase(it);
//     }
//     else
//     {
//         throw std::runtime_error("Received duplicate_error that was not addressed to me.");
//     }
// }









// tRoutingEntryVec Node::findDestinationEntries(tPacketptr packet) {
//     tRoutingEntryVec entries;

//     for_each(routingTable.begin(), routingTable.end(), [&](std::shared_ptr<RoutingEntry> entry) {
//         if (entry->destinationAddress == packet->destinationAddress) {
//             entries.push_back(entry);
//         }
//     });

//     return entries;
// }

// std::shared_ptr<RoutingEntry> Node::getEntryForDestinationAndHop(int dest, int hop) {
//     std::shared_ptr<RoutingEntry> entry = NULL;

//     std::for_each(routingTable.begin(), routingTable.end(), [&](std::shared_ptr<RoutingEntry> routingEntry) {
//         if (routingEntry->destinationAddress == dest && routingEntry->nextHopAddress == hop) {
//             entry = routingEntry;
//         }
//     });

//     return entry;
// }






//liczba bloków to liczba węzłów. Węzłów może być aż do 2^16-1, czyli 65535
//liczba wątków to maksymalna liczba pakietów do przetworzenia na raz. Moja CUDA umożliwia stworzenie 1024 wątków.
int main()
{
    std::srand(std::time(0));

    PacketBuffer incomming_buffer[nodes_num][nodes_num][nodes_num]; //Should this also be a thrust::device_vector ?
    PacketBuffer outgoing_buffer[nodes_num][nodes_num][nodes_num];
    RoutingEntry routing_table[nodes_num][nodes_num][nodes_num];

    //TODO initialize values
    // routing_table[1][4][2].pheromone = 0.5;
    // routing_table[2][4][3].pheromone = 0.5;
    // routing_table[3][4][4].pheromone = 0.5;


    PacketBuffer* device_incomming_buffer_ptr;
    PacketBuffer* device_outgoing_buffer_ptr;
    RoutingEntry* device_routing_table_ptr;







    // cudaMalloc(&device_incomming_buffer_ptr, total_elem_num*sizeof(PacketBuffer));
    // cudaMalloc(&device_outgoing_buffer_ptr, total_elem_num*sizeof(PacketBuffer));
    // cudaMalloc(&device_routing_table_ptr, total_elem_num*sizeof(RoutingEntry));

    // cudaMemcpy(incomming_buffer, device_incomming_buffer_ptr, total_elem_num*sizeof(PacketBuffer), cudaMemcpyHostToDevice);
    // cudaMemcpy(outgoing_buffer, device_outgoing_buffer_ptr, total_elem_num*sizeof(PacketBuffer), cudaMemcpyHostToDevice);
    // cudaMemcpy(routing_table, device_routing_table_ptr, total_elem_num*sizeof(RoutingEntry), cudaMemcpyHostToDevice);


    device_incomming_buffer_ptr = (PacketBuffer* ) malloc(total_elem_num*sizeof(PacketBuffer));
    device_outgoing_buffer_ptr = (PacketBuffer* ) malloc(total_elem_num*sizeof(PacketBuffer));
    device_routing_table_ptr = (RoutingEntry* ) malloc(total_elem_num*sizeof(RoutingEntry));

    memcpy(device_incomming_buffer_ptr, incomming_buffer, total_elem_num*sizeof(PacketBuffer));
    memcpy(device_outgoing_buffer_ptr, outgoing_buffer, total_elem_num*sizeof(PacketBuffer));
    memcpy(device_routing_table_ptr, routing_table, total_elem_num*sizeof(RoutingEntry));


    const unsigned from = 1;
    const unsigned to = 9;
    int thread_num = 3;

    unsigned packet_sequence = 0;

    for(int ticks=0; ticks<10000; ++ticks)
    {
        if (ticks%3 == 0)
        {
            PacketBuffer& buffer = device_incomming_buffer_ptr[calcElem(from, to, from)];
            Packet packet(from, packet_sequence++);
            buffer.addPacket(packet);

            PacketBuffer& buffer2 = device_incomming_buffer_ptr[calcElem(to, from, to)];
            Packet packet1(to, packet_sequence++);
            buffer2.addPacket(packet1);
        }

    //     nodesTick<<<nodes_num, 5>>>(device_incomming_buffer_ptr, 
    //                                 device_outgoing_buffer_ptr, 
    //                                 device_routing_table_ptr);
        for (int j = 0 ; j<nodes_num; ++j)
        {
            for (int i = 0 ; i<thread_num; ++i)
            {
                nodesTick(device_incomming_buffer_ptr, 
                          device_outgoing_buffer_ptr, 
                          device_routing_table_ptr, 
                          i, j, thread_num, nodes_num);
            }
        }
        
        std::swap(device_incomming_buffer_ptr, 
                  device_outgoing_buffer_ptr);
    }




    // cudaMemcpy(incomming_buffer, device_incomming_buffer_ptr, total_elem_num*sizeof(PacketBuffer), cudaMemcpyDeviceToHost);
    // cudaMemcpy(outgoing_buffer, device_outgoing_buffer_ptr, total_elem_num*sizeof(PacketBuffer), cudaMemcpyDeviceToHost);
    // cudaMemcpy(routing_table, device_routing_table_ptr, total_elem_num*sizeof(RoutingEntry), cudaMemcpyDeviceToHost);

    memcpy(incomming_buffer, device_incomming_buffer_ptr, total_elem_num*sizeof(PacketBuffer));
    memcpy(outgoing_buffer, device_outgoing_buffer_ptr, total_elem_num*sizeof(PacketBuffer));
    memcpy(routing_table, device_routing_table_ptr, total_elem_num*sizeof(RoutingEntry));

    //check the results
    std::cout << "Routing table:\n";
    for (int i=0; i<nodes_num; ++i)
    {
        std::cout << "NODE: " << i << "\n";
        for (int j=0; j<nodes_num; ++j)
        {
            std::cout << "\ttarget: " << j << "\n";
            for (int k=0; k<nodes_num; ++k)
            {
                std::cout << "\t\tneghbour: " << k << " - " << routing_table[i][j][k].pheromone << "\n";
            }
        }
    }
    std::cout << std::flush;


    



    // cudaFree(device_incomming_buffer_ptr);
    // cudaFree(device_outgoing_buffer_ptr);
    // cudaFree(device_routing_table_ptr);

    free(device_incomming_buffer_ptr);
    free(device_outgoing_buffer_ptr);
    free(device_routing_table_ptr);
}