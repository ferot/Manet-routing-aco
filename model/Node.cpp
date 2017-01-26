#include "Node.hpp"
#include <thrust/device_vector.h>
#include <thrust/host_vector.h>
#include <curand.h>
#include <curand_kernel.h>
#define MAX 100000
#define SIZE 100


constexpr int nodes_num = 5;

typedef thrust::device_vector<Packet> PacketBuffer;

struct PacketBuffer
{
    int size_;
    int num_of_elements_;
    Packet* buffer_;
}


__device__ int drawNextHop(RoutingEntry* routing_for_neighbours, int neighbours_num)
{
    float pheromoneSum = 0.0;

    //count sum of pheromones for available routes
    for (int i=0; i < neighbours_num; ++i)
    {
        float pheromone = routing_for_neighbours[i].pheromone;
        if (pheromone > 0.0)
        {
            pheromoneSum += pheromone;
        }
    }

    curandState_t state; //TODO initialize this only once
    curand_init(pheromoneSum, 0, 0, &state);
    result = curand(&state) % MAX;
    float random = (result/(float)MAX) * pheromoneSum;

    int choosenHop = -1;
    while(random >= 0.0) {
        ++choosenHop;
        random -= routing_for_neighbours[choosenHop].pheromone;
    }

    return choosenHop;
}


// TO się musi wykonać sekwencyjnie. Czasu nie oszukasz. Ale może obejdzie się bez kopiowania do hosta
// [node][target][neighbour]
__global__ void nodesTick(PacketBuffer* incomming_buffers[][nodes_num][nodes_num], 
                          PacketBuffer* outgoing_buffers[][nodes_num][nodes_num], RoutingEntry* routing_table[][nodes_num][nodes_num])
{
    __shared__ RoutingEntry[blockDim.x][blockDim.x] local_routing_table;


    int node = blockIdx.x;
    for (int target = threadIdx.x; target < blockDim.x; target += threadDim.x)
    {
        //Copying local routing table
        memcpy(local_routing_table[target], 
               routing_table[node][target], 
               blockDim.x * sizeof(RoutingEntry));

        // updating whole routing table. TODO Do we have to synchronize after that? Packet never change its target.
        for (int neighbour = 0; neighbour < blockDim.x; ++neighbour)
        {
            if (local_routing_table[target][neighbour].pheromone > 0.0)
            {
                routing_entry.evaporatePheromone();
            }
        }

        for (int prev_hop = 0; prev_hop < blockDim.x; ++prev_hop) //neighbours from which we reived packets
        {
            if (local_routing_table[target][prev_hop].pheromone < 0.0)
                continue; //Not a neighbour


            PacketBuffer& current_incoming_packet_buffer = incomming_buffers[node][target][prev_hop];

            if (node != target)
            {
                for (int i=0; i< current_incoming_packet_buffer.num_of_elements_; ++i)
                {
                    Packet& current_packet = current_incoming_packet_buffer.buffer_[i];
                    // TODO just now only regular packets
                    // processPacket(node, current_packet); //TODO where to copy (send)? Potentially many next hops.

                    //Increase pheromone for the previous hop
                    local_routing_table[current_packet.sourceAddress][prev_hop].increasePheromone();

                    // TODO temporarly here
                    int next_hop = drawNextHop(local_routing_table[target], blockDim.x);
                    current_packet.hops_count += 1;

                    // sending packet
                    PacketBuffer& outgoing_packet_buffer = outgoing_buffers[next_hop][target][node].push_back(current_packet);

                    if (outgoing_packet_buffer.size_ == outgoing_packet_buffer.num_of_elements_)
                    {
                        Packet* new_bigger_buffer = new Packet[outgoing_packet_buffer.size_ + SIZE];
                        memcopy(outgoing_packet_buffer.buffer_, 
                                new_bigger_buffer, 
                                outgoing_packet_buffer.num_of_elements_ * sizeof(Packet));
                        delete[] outgoing_packet_buffer.buffer_;
                        outgoing_packet_buffer.buffer_ = new_bigger_buffer;
                        outgoing_packet_buffer.size_ += SIZE;
                    }

                    outgoing_packet_buffer.buffer_[outgoing_packet_buffer.num_of_elements_] = current_packet;
                    outgoing_packet_buffer.num_of_elements_ += 1;

                    local_routing_table[target][next_hop].increasePheromone();
                }
            }
            
            //free incoming buffer
            delete[] current_incoming_packet_buffer.buffer_;
            current_incoming_packet_buffer.buffer_ = new Packet[SIZE];
            current_incoming_packet_buffer.size_ = SIZE;
            current_incoming_packet_buffer.num_of_elements_ = 0;
        }
        
        memcpy(routing_table[node][target],
               my_routing_table[target], 
               blockDim.x * sizeof(RoutingEntry));
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
    PacketBuffer incomming_buffer[nodes_num][nodes_num][nodes_num]; //Should this also be a thrust::device_vector ?
    PacketBuffer outgoing_buffer[nodes_num][nodes_num][nodes_num];
    RoutingEntry routing_table[nodes_num][nodes_num][nodes_num];
    routing_table[1][4][2] = 0.5;
    routing_table[2][4][3] = 0.5;
    routing_table[3][4][3] = 0.5;

    //TODO initialize values
    thrust::host_vector<Packet> initial_packets;
    initial_packets.emplace(1, 0);
    incomming_buffer[1][4][1] = initial_packets;

    PacketBuffer* device_incomming_buffer_ptr[][nodes_num][nodes_num];
    PacketBuffer* device_outgoing_buffer_ptr[][nodes_num][nodes_num];
    RoutingEntry* device_routing_table_ptr[][nodes_num][nodes_num];

    cudaMalloc(&device_incomming_buffer_ptr, std::pow(nodes_num, 3)*sizeof(PacketBuffer));
    cudaMalloc(&device_outgoing_buffer_ptr, std::pow(nodes_num, 3)*sizeof(PacketBuffer));
    cudaMalloc(&device_routing_table_ptr, std::pow(nodes_num, 3)*sizeof(RoutingEntry));

    cudaMemcpy(incomming_buffer, device_incomming_buffer_ptr, std::pow(nodes_num, 3)*sizeof(PacketBuffer), cudaMemcpyHostToDevice);
    cudaMemcpy(outgoing_buffer, device_outgoing_buffer_ptr, std::pow(nodes_num, 3)*sizeof(PacketBuffer), cudaMemcpyHostToDevice);
    cudaMemcpy(routing_table, device_routing_table_ptr, std::pow(nodes_num, 3)*sizeof(RoutingEntry), cudaMemcpyHostToDevice);





    for(unsigned i=0; i<3; ++i)
    {
        nodesTick<<<nodes_num, 5>>>(device_incomming_buffer_ptr, 
                                    device_outgoing_buffer_ptr, 
                                    device_routing_table_ptr);
        std::swap(device_incomming_buffer_ptr, 
                  device_outgoing_buffer_ptr);
    }




    cudaMemcpy(incomming_buffer, device_incomming_buffer_ptr, std::pow(nodes_num, 3)*sizeof(PacketBuffer), cudaMemcpyDeviceToHost);
    cudaMemcpy(outgoing_buffer, device_outgoing_buffer_ptr, std::pow(nodes_num, 3)*sizeof(PacketBuffer), cudaMemcpyDeviceToHost);
    cudaMemcpy(routing_table, device_routing_table_ptr, std::pow(nodes_num, 3)*sizeof(RoutingEntry), cudaMemcpyDeviceToHost);

    //TODO check the results



    



    cudaFree(device_incomming_buffer_ptr);
    cudaFree(device_outgoing_buffer_ptr);
    cudaFree(device_routing_table_ptr);
}