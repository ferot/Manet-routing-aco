// #include <curand.h>
// #include <curand_kernel.h>

#include <mpi.h>
#include <time.h>
#include <cstdlib>
#include <ctime>
#include <cmath>
#include <cstring>
#include <memory>
#include <iostream>
#include <vector>
#include <algorithm>
#include <fstream>
#include "Packet.h"
#include "RoutingEntry.hpp"
#include "json.hpp"

constexpr int MAX = 100000;
constexpr int BUFFER_STEP = 10;
constexpr int nodes_num = 90;
constexpr long total_elem_num = nodes_num * nodes_num * nodes_num;
constexpr long buffers_elem_num = nodes_num * nodes_num;

typedef struct {
        int procNo, pathLength;
} MRA_Result;

class PacketBuffer
{
public:
    
    unsigned size_;
    unsigned num_of_elements_;
    Packet* buffer_;

    PacketBuffer() : num_of_elements_(0), size_(0), buffer_(NULL) {}
    ~PacketBuffer() 
    {
        if (buffer_ != NULL)
        {
            delete[] buffer_;
        }
    }

    void addPacket(Packet& packet)
    {
        if (buffer_ == NULL)
        {
            size_ = BUFFER_STEP;
            buffer_ = new Packet[size_];
        }
        else if (num_of_elements_ == size_)
        {
            size_ += BUFFER_STEP;
            Packet* new_buffer = new Packet[size_];
            memcpy(new_buffer, buffer_, num_of_elements_);
            delete[] buffer_;
            buffer_ = new_buffer;
        }

        buffer_[num_of_elements_] = packet;
        num_of_elements_ += 1;
    }

    void reset()
    {
        delete[] buffer_;
        buffer_ = NULL;
        size_ = 0;
        num_of_elements_ = 0;
    }
};

/*__device__*/ 
int calcElem(int node, int target, int neighbour)
{
    return (node*nodes_num + target)*nodes_num  + neighbour;
}

int calcBufferElem(int node, int neighbour)
{
    return node*nodes_num + neighbour;
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

template<class K>
void atomicAdd(K* where, K what)
{
    (*where) += what;
}


// TO się musi wykonać sekwencyjnie. Czasu nie oszukasz. Ale może obejdzie się bez kopiowania do hosta
// [node][target][neighbour]

/*__global__*/ 
void nodesTick(PacketBuffer* incomming_buffers, 
               PacketBuffer* outgoing_buffers, 
               RoutingEntry* routing_table,
               int thread_num, int block_num, int thread_dim, int block_dim,
               unsigned stage, int from, int to, unsigned tick)
{
    PseudoCuda blockIdx, blockDim, threadIdx, threadDim;
    blockIdx.x = block_num;
    blockDim.x = block_dim;
    threadIdx.x = thread_num;
    threadDim.x = thread_dim;

    const int node = blockIdx.x;

    
    switch (stage) 
    {
    case 0:
    {
        //TODO add packet generation randomly
        if (node == from && threadIdx.x == 0 && rand()%3 == 0)
        {
            PacketBuffer& buffer = incomming_buffers[calcBufferElem(from, from)];
            Packet packet(from, to, tick);
            buffer.addPacket(packet);
        }
        
        if (node == to && threadIdx.x == 0 && rand()%3 == 0)
        {
            PacketBuffer& buffer = incomming_buffers[calcBufferElem(to, to)];
            Packet packet(to, from, tick);
            buffer.addPacket(packet);
        }
        
    }
    break;
    case 1:
    {
        // Ewaporacja tablicy feromonów
        int start_elem = node * nodes_num * nodes_num;
        int end_elem = start_elem + nodes_num * nodes_num;
        for (int elem = start_elem + threadIdx.x; elem < end_elem; elem += threadDim.x)
        {
            RoutingEntry& routing_entry = routing_table[elem];
            if (routing_entry.pheromone > 0.0)
            {
                routing_entry.evaporatePheromone();
            }
        }

    }
    break;
    case 2: // __synchronize();
    {
        // Updating next_hops TODO could take packets more evenly
        for (int prev_hop = 0; prev_hop < blockDim.x; ++prev_hop) //neighbours from which we reived packets
        {
            PacketBuffer& current_incoming_packet_buffer = incomming_buffers[calcBufferElem(node, prev_hop)];

            for (int i=threadIdx.x; i< current_incoming_packet_buffer.num_of_elements_; i += threadDim.x)
            {
                Packet& current_packet = current_incoming_packet_buffer.buffer_[i];

                if (prev_hop != node)
                {
                    float pheromone_increasement = RoutingEntry::calculateIncreasement(current_packet.hops_count);
                    RoutingEntry& routing_entry = routing_table[calcElem(node, current_packet.sourceAddress, prev_hop)];
                    atomicAdd(&routing_entry.pheromone, pheromone_increasement);
                }

                if (node == current_packet.destinationAddress)
                {
                    //Since current_packet.next_hop == node, this packet won't be copied any more.

                    // std::cout << "Packet " << current_packet.sequenceNumber << " reached node " << node << " after " << current_packet.hops_count << " hops." << std::endl;
                }
                else
                {
                    int next_hop = drawNextHop(routing_table, node, current_packet.destinationAddress);
                    current_packet.hops_count += 1;
                    current_packet.next_hop = next_hop;

                    float pheromone_increasement = RoutingEntry::calculateIncreasement(current_packet.hops_count);
                    RoutingEntry& routing_entry = routing_table[calcElem(node, current_packet.destinationAddress, next_hop)];
                    atomicAdd(&routing_entry.pheromone, pheromone_increasement);

                    // std::cout << "At node " << node << " packet " << current_packet.sequenceNumber << " whose target is " << current_packet.destinationAddress << " came from " << prev_hop << " and will be send to " << next_hop << "." << std::endl;

                }

            }
        }

    }
    break;
    case 3: // __synchronize();
    {

        // Czyszczenie buforów wyjściowych i wysyłanie pakietów
        for (int next_hop = threadIdx.x; next_hop < blockDim.x; next_hop += threadDim.x)
        {
            PacketBuffer& outgoing_packet_buffer = outgoing_buffers[calcBufferElem(next_hop, node)];
            outgoing_packet_buffer.reset();

            for (int prev_hop = 0; prev_hop < blockDim.x; ++prev_hop) //neighbours from which we reived packets
            {
                PacketBuffer& incoming_packet_buffer = incomming_buffers[calcBufferElem(node, prev_hop)];

                for (int i=0; i< incoming_packet_buffer.num_of_elements_; ++i)
                {
                    Packet& current_packet = incoming_packet_buffer.buffer_[i];

                    if (current_packet.next_hop == next_hop && current_packet.destinationAddress != node)
                    {
                        outgoing_packet_buffer.addPacket(current_packet);
                        // std::cout << "At node " << node << " packet " << current_packet.sequenceNumber << " whose target is " << current_packet.destinationAddress << " came from " << prev_hop << " and goes to " << next_hop << "." << std::endl;
                    }
                }
            }
        } 

    }
    break;
    case 4: // __synchronize()
    {

        //free incoming buffer
        for (int prev_hop = threadIdx.x; prev_hop < blockDim.x; prev_hop += threadDim.x)
        {
            PacketBuffer& current_incoming_packet_buffer = incomming_buffers[calcBufferElem(node, prev_hop)];
            current_incoming_packet_buffer.reset();
        }
    }
    break;
    default:
        assert(false);
    }

    
}





int printBestPath(int from, int to, RoutingEntry routing_table[nodes_num][nodes_num][nodes_num])
{
    int counter = 0;
    std::vector<int> visited;
    int current_hop = from;
    std::cout << current_hop << ", ";
    while (current_hop != to)
    {
        if (std::find(visited.begin(), visited.end(), current_hop) != visited.end())
        {
            std::cout << ", Loop";
            counter = 999;
            break;
        }
        visited.push_back(current_hop);

        int best_hop = -1;
        float best_pheromone = -10000.0f;
        for (int i=0; i < nodes_num; ++i)
        {
            float pheromone = routing_table[current_hop][to][i].pheromone;
            if (pheromone > best_pheromone)
            {
                best_hop = i;
                best_pheromone = pheromone;
            }
        }

        current_hop = best_hop;
        counter += 1;
        std::cout << current_hop << ", ";
    }
    std::cout << " (Path length: " << counter << ")" << std::endl;
    return counter;
}

//does the same as printBestPath just no cout - needed to count fast path lentgth w-out printing
int countBestPath(int from, int to, RoutingEntry routing_table[nodes_num][nodes_num][nodes_num])
{
    int counter = 0;
    std::vector<int> visited;
    int current_hop = from;
    while (current_hop != to)
    {
        if (std::find(visited.begin(), visited.end(), current_hop) != visited.end())
        {
            counter = 999;
            break;
        }
        visited.push_back(current_hop);

        int best_hop = -1;
        float best_pheromone = -10000.0f;
        for (int i=0; i < nodes_num; ++i)
        {
            float pheromone = routing_table[current_hop][to][i].pheromone;
            if (pheromone > best_pheromone)
            {
                best_hop = i;
                best_pheromone = pheromone;
            }
        }

        current_hop = best_hop;
        counter += 1;
    }
    return counter;
}


int getShortestDist(){
    int shortest = 0;
    std::ifstream myReadFile;
     myReadFile.open("shortestDist.txt");
     char output[100];
     if (myReadFile.is_open()) {
     while (!myReadFile.eof()) {
        myReadFile >> output;
//        std::cout<<output;
     }
    }
     std::stringstream strValue;
     strValue << output;
     strValue >> shortest;
    myReadFile.close();
    return shortest;
}


//liczba bloków to liczba węzłów. Węzłów może być aż do 2^16-1, czyli 65535
//liczba wątków to maksymalna liczba pakietów do przetworzenia na raz. Moja CUDA umożliwia stworzenie 1024 wątków.
int main(int argc, char **argv)
{
    MPI_Datatype MPI_RESULT;

    MPI_Init(&argc, &argv);
    std::srand(std::time(0));

    // Get the number of processes
    int world_size;
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);

    // Get the rank of the process
    int rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    // Build MPI_RESULT datatype for MRA_Result
    MPI_Type_contiguous(2, MPI_INT, &MPI_RESULT);
    MPI_Type_commit(&MPI_RESULT);

//    std::cout << shortestDist << std::endl;

    PacketBuffer incomming_buffer[nodes_num][nodes_num];
    PacketBuffer outgoing_buffer[nodes_num][nodes_num];
    RoutingEntry routing_table[nodes_num][nodes_num][nodes_num];

    short int shortestDist =  getShortestDist();

    std::string fileName = "graph.json";
    if (argc == 3) {
        shortestDist = atoi(argv[1]);
        std::string newFilename(argv[2]);
        fileName = newFilename;
    }

    std::cout << fileName << std::endl;

    // initialize values
    std::ifstream inputStream(fileName.c_str());
    nlohmann::json graphJSON;
    inputStream >> graphJSON;

    for (auto jsonEdge : graphJSON["edges"]) {
        int fromNodeNumber = jsonEdge[0];
        int toNodeNumber = jsonEdge[1];

        for (int i=0; i < nodes_num; ++i)
        {
            routing_table[fromNodeNumber][i][toNodeNumber].pheromone = 1.0;
            routing_table[toNodeNumber][i][fromNodeNumber].pheromone = 1.0;
        }
    }

    PacketBuffer* device_incomming_buffer_ptr;
    PacketBuffer* device_outgoing_buffer_ptr;
    RoutingEntry* device_routing_table_ptr;

    device_incomming_buffer_ptr = (PacketBuffer* ) malloc(buffers_elem_num*sizeof(PacketBuffer));
    device_outgoing_buffer_ptr = (PacketBuffer* ) malloc(buffers_elem_num*sizeof(PacketBuffer));
    device_routing_table_ptr = (RoutingEntry* ) malloc(total_elem_num*sizeof(RoutingEntry));

    memcpy(device_incomming_buffer_ptr, incomming_buffer, buffers_elem_num*sizeof(PacketBuffer));
    memcpy(device_outgoing_buffer_ptr, outgoing_buffer, buffers_elem_num*sizeof(PacketBuffer));
    memcpy(device_routing_table_ptr, routing_table, total_elem_num*sizeof(RoutingEntry));

    // Capture the starting time
    MPI_Barrier(MPI_COMM_WORLD);
    double start = 0.0, totalTime = 0.0;
    start = MPI_Wtime();

    std::srand(std::time(0) + rank);
    const unsigned from = 0;
    const unsigned to = 2;
    int thread_num = 1;

    unsigned sequence = 0;

    for(int ticks=0; ticks<3000; ++ticks)
    {
        sequence += 1;
        if (ticks%200 == 0)
        {
//            std::cout << "Process :: " << rank << " ";
            memcpy(routing_table, device_routing_table_ptr, total_elem_num*sizeof(RoutingEntry));
            int path = countBestPath(from, to, routing_table);
            if (totalTime == 0.0 && path == shortestDist) {
                double finishedTime = MPI_Wtime();
                totalTime = finishedTime - start;
                std::cout << "Najlepszy czas" << totalTime << std::endl;
            }
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
                          i, j, thread_num, nodes_num,
                          0, from, to, sequence);
            }
            for (int i = 0 ; i<thread_num; ++i)
            {
                nodesTick(device_incomming_buffer_ptr,
                          device_outgoing_buffer_ptr,
                          device_routing_table_ptr,
                          i, j, thread_num, nodes_num,
                          1, from, to, sequence);
            }
            for (int i = 0 ; i<thread_num; ++i)
            {
                nodesTick(device_incomming_buffer_ptr,
                          device_outgoing_buffer_ptr,
                          device_routing_table_ptr,
                          i, j, thread_num, nodes_num,
                          2, from, to, sequence);
            }
            for (int i = 0 ; i<thread_num; ++i)
            {
                nodesTick(device_incomming_buffer_ptr,
                          device_outgoing_buffer_ptr,
                          device_routing_table_ptr,
                          i, j, thread_num, nodes_num,
                          3, from, to, sequence);
            }
            for (int i = 0 ; i<thread_num; ++i)
            {
                nodesTick(device_incomming_buffer_ptr,
                          device_outgoing_buffer_ptr,
                          device_routing_table_ptr,
                          i, j, thread_num, nodes_num,
                          4, from, to, sequence);
            }
        }
        
        std::swap(device_incomming_buffer_ptr, 
                  device_outgoing_buffer_ptr);
    }

    memcpy(incomming_buffer, device_incomming_buffer_ptr, buffers_elem_num*sizeof(PacketBuffer));
    memcpy(outgoing_buffer, device_outgoing_buffer_ptr, buffers_elem_num*sizeof(PacketBuffer));
    memcpy(routing_table, device_routing_table_ptr, total_elem_num*sizeof(RoutingEntry));

    //Process prepares his results to send
    MRA_Result result;
    result.procNo = rank;
    result.pathLength = countBestPath(from, to, routing_table);

    //gather all pathLengths to decide which one prints result
    MRA_Result *pathLengths = NULL;
    if (!rank) {
        pathLengths = (MRA_Result*)malloc(sizeof(MRA_Result) * world_size);
    }

    MPI_Gather(&result,         //send_data
               1,               //send_count
               MPI_RESULT,      //send_datatype
               pathLengths,     //recv_data
               1,               //recv_count
               MPI_RESULT,      //recv_datatype
               0,               //root
               MPI_COMM_WORLD); //communicator

    //master process checks results
    MRA_Result bestResult;
    if (!rank) {
        bestResult = result;
        for(int i = 0; i < world_size; i++) {
            if (bestResult.pathLength > pathLengths[i].pathLength) {
                bestResult.pathLength = pathLengths[i].pathLength;
                bestResult.procNo = pathLengths[i].procNo;
            }
        }
    }

    //master broadcasts results to other processes
    MPI_Bcast(&bestResult, 1, MPI_RESULT, 0, MPI_COMM_WORLD);

    // Capture the ending time
    MPI_Barrier(MPI_COMM_WORLD);

    if (bestResult.procNo == rank) {
        std::cout << "Najlepsza sciezka: ";
        printBestPath(from, to, routing_table);
//        printf("\nTime spent (%.15f)", doubleResult / 10000);
    }


    free(device_incomming_buffer_ptr);
    free(device_outgoing_buffer_ptr);
    free(device_routing_table_ptr);

    MPI_Type_free(&MPI_RESULT);
    MPI_Finalize();
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
