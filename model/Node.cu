// #include <curand.h>
// #include <curand_kernel.h>

#include <cstdlib>
#include <ctime>
#include <cmath>
#include <cstring>
#include <memory>
#include <iostream>
#include <vector>
#include <algorithm>
#include <iomanip>
#include <curand_kernel.h>

#include "Packet.h"
#include "RoutingEntry.hpp"
#include "config.h"
#include "cudaError.h"


class PacketBuffer
{
public:
    
    unsigned size_;
    unsigned num_of_elements_;
    Packet* buffer_;

    __device__
    PacketBuffer() : num_of_elements_(0), size_(0), buffer_(NULL) {}

    __device__
    ~PacketBuffer() 
    {
        if (buffer_ != NULL)
        {
            delete[] buffer_;
        }
    }

    __device__
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

    __device__
    void reset()
    {
        delete[] buffer_;
        buffer_ = NULL;
        size_ = 0;
        num_of_elements_ = 0;
    }
};

__device__ 
int calcElem(int node, int target, int neighbour)
{
    return (node*nodes_num + target)*nodes_num  + neighbour;
}

__device__
int calcBufferElem(int node, int neighbour)
{
    return node*nodes_num + neighbour;
}


__device__
int drawNextHop(RoutingEntry* routing_for_neighbours, int node, int target, curandState& state)
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

    float part = curand_uniform(&state);
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



// TO się musi wykonać sekwencyjnie. Czasu nie oszukasz. Ale może obejdzie się bez kopiowania do hosta
// [node][target][neighbour]

__global__
void initializePacketBuffers(PacketBuffer* incomming_buffers, 
                             PacketBuffer* outgoing_buffers)
{
    const int node = blockIdx.x;
    for (int i=threadIdx.x; i < gridDim.x; i += blockDim.x)
    {
        new (incomming_buffers + calcBufferElem(node, i)) PacketBuffer();
        new (outgoing_buffers + calcBufferElem(i, node)) PacketBuffer();
    }
}

__global__
void deinitializePacketBuffers(PacketBuffer* incomming_buffers, 
                               PacketBuffer* outgoing_buffers)
{
    const int node = blockIdx.x;
    for (int i=threadIdx.x; i < gridDim.x; i += blockDim.x)
    {
        incomming_buffers[calcBufferElem(node, i)].~PacketBuffer();
        outgoing_buffers[calcBufferElem(i, node)].~PacketBuffer();
    }
}

__global__
void nodesTick(PacketBuffer* incomming_buffers, 
               PacketBuffer* outgoing_buffers, 
               RoutingEntry* routing_table,
               int from, int to, unsigned tick)
{
    const int node = blockIdx.x;
    int tId = threadIdx.x + (blockIdx.x * blockDim.x);
    curandState state;
    curand_init((unsigned long long)clock() + tId, 0, 0, &state);

    
    //TODO add packet generation randomly
    if (node == from && threadIdx.x == 0 && tick%5 == 0)
    {
        PacketBuffer& buffer = incomming_buffers[calcBufferElem(from, from)];
        Packet packet(from, to, tick);
        buffer.addPacket(packet);
    }
    
    if (node == to && threadIdx.x == 0 && tick%5 == 0)
    {
        PacketBuffer& buffer = incomming_buffers[calcBufferElem(to, to)];
        Packet packet(to, from, tick);
        buffer.addPacket(packet);
    }
        

    __syncthreads();


    // Ewaporacja tablicy feromonów
    int start_elem = node * nodes_num * nodes_num;
    int end_elem = start_elem + nodes_num * nodes_num;
    for (int elem = start_elem + threadIdx.x; elem < end_elem; elem += blockDim.x)
    {
        RoutingEntry& routing_entry = routing_table[elem];
        if (routing_entry.pheromone > 0.0)
        {
            routing_entry.evaporatePheromone();
        }
    }


    __syncthreads();
    

    // Updating next_hops TODO could take packets more evenly
    for (int prev_hop = 0; prev_hop < gridDim.x; ++prev_hop) //neighbours from which we reived packets
    {
        PacketBuffer& current_incoming_packet_buffer = incomming_buffers[calcBufferElem(node, prev_hop)];

        for (int i=threadIdx.x; i< current_incoming_packet_buffer.num_of_elements_; i += blockDim.x)
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
                int next_hop = drawNextHop(routing_table, node, current_packet.destinationAddress, state);
                current_packet.hops_count += 1;
                current_packet.next_hop = next_hop;

                float pheromone_increasement = RoutingEntry::calculateIncreasement(current_packet.hops_count);
                RoutingEntry& routing_entry = routing_table[calcElem(node, current_packet.destinationAddress, next_hop)];
                atomicAdd(&routing_entry.pheromone, pheromone_increasement);

                // std::cout << "At node " << node << " packet " << current_packet.sequenceNumber << " whose target is " << current_packet.destinationAddress << " came from " << prev_hop << " and will be send to " << next_hop << "." << std::endl;

            }

        }
    }

   
    __syncthreads();
    

    // Czyszczenie buforów wyjściowych i wysyłanie pakietów
    for (int next_hop = threadIdx.x; next_hop < gridDim.x; next_hop += blockDim.x)
    {
        PacketBuffer& outgoing_packet_buffer = outgoing_buffers[calcBufferElem(next_hop, node)];
        outgoing_packet_buffer.reset();

        for (int prev_hop = 0; prev_hop < gridDim.x; ++prev_hop) //neighbours from which we reived packets
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

    
    __syncthreads();
    

    //free incoming buffer
    for (int prev_hop = threadIdx.x; prev_hop < gridDim.x; prev_hop += blockDim.x)
    {
        PacketBuffer& current_incoming_packet_buffer = incomming_buffers[calcBufferElem(node, prev_hop)];
        current_incoming_packet_buffer.reset();
    }

}














int printBestPath(int from, int to, RoutingEntry (&routing_table)[nodes_num][nodes_num][nodes_num])
{
    int counter = 0;
    std::vector<int> visited;
    int current_hop = from;
    std::cout << current_hop << ", ";
    while (current_hop != to)
    {
        if (std::find(visited.begin(), visited.end(), current_hop) != visited.end())
        {
            std::cout << "Loop";
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

void printRoutingTable(RoutingEntry (&routing_table)[nodes_num][nodes_num][nodes_num])
{
    for (int node=0; node < nodes_num; ++node)
    {
        std::cout << "\n\nNODE: " << node;
        for (int target=0; target < nodes_num; ++target)
        {
            std::cout << "\n  target " << target << ": ";
            for (int neighbour=0; neighbour < nodes_num; ++neighbour)
            {
                // std::cout.width(9);
                std::cout << std::setw( 11 ) << routing_table[node][target][neighbour].pheromone << " ";
            }
        }
    }
    std::cout << std::endl;
}


void initializeRoutingTable(std::string file_name, RoutingEntry (&routing_table)[nodes_num][nodes_num][nodes_num]);

//liczba bloków to liczba węzłów. Węzłów może być aż do 2^16-1, czyli 65535
//liczba wątków to maksymalna liczba pakietów do przetworzenia na raz. Moja CUDA umożliwia stworzenie 1024 wątków.
int main()
{
    // PacketBuffer incomming_buffer[nodes_num][nodes_num];
    // PacketBuffer outgoing_buffer[nodes_num][nodes_num];
    RoutingEntry routing_table[nodes_num][nodes_num][nodes_num];

    // initialize values
    initializeRoutingTable("graph.json", routing_table);




    PacketBuffer* device_incomming_buffer_ptr;
    PacketBuffer* device_outgoing_buffer_ptr;
    RoutingEntry* device_routing_table_ptr;

    CudaSafeCall( cudaMalloc(&device_incomming_buffer_ptr ,buffers_elem_num*sizeof(PacketBuffer)) );
    CudaSafeCall( cudaMalloc(&device_outgoing_buffer_ptr, buffers_elem_num*sizeof(PacketBuffer)) );
    CudaSafeCall( cudaMalloc(&device_routing_table_ptr, total_elem_num*sizeof(RoutingEntry)) );

    // cudaMemcpy(device_incomming_buffer_ptr, incomming_buffer, buffers_elem_num*sizeof(PacketBuffer), cudaMemcpyHostToDevice);
    // cudaMemcpy(device_outgoing_buffer_ptr, outgoing_buffer, buffers_elem_num*sizeof(PacketBuffer), cudaMemcpyHostToDevice);
    CudaSafeCall( cudaMemcpy(device_routing_table_ptr, routing_table, total_elem_num*sizeof(RoutingEntry), cudaMemcpyHostToDevice) );




    const unsigned from = 0;
    const unsigned to = 2;

    initializePacketBuffers<<<nodes_num, thread_num>>>(device_incomming_buffer_ptr, 
                                                       device_outgoing_buffer_ptr);
    CudaCheckError();

    for(int ticks=0; ticks<10000; ++ticks)
    {
        if (ticks%200 == 0)
        {
            CudaSafeCall( cudaMemcpy(routing_table, device_routing_table_ptr, total_elem_num*sizeof(RoutingEntry), cudaMemcpyDeviceToHost) );
            printBestPath(from, to, routing_table);
        }


        nodesTick<<<nodes_num, thread_num>>>(device_incomming_buffer_ptr, 
                                             device_outgoing_buffer_ptr, 
                                             device_routing_table_ptr,
                                             from, to, ticks);
        CudaCheckError();
        // cudaDeviceSynchronize();
        
        std::swap(device_incomming_buffer_ptr, 
                  device_outgoing_buffer_ptr);
    }

    deinitializePacketBuffers<<<nodes_num, thread_num>>>(device_incomming_buffer_ptr, 
                                                         device_outgoing_buffer_ptr);
    CudaCheckError();


    // cudaMemcpy(device_incomming_buffer_ptr, incomming_buffer, buffers_elem_num*sizeof(PacketBuffer), cudaMemcpyDeviceToHost);
    // cudaMemcpy(device_outgoing_buffer_ptr, outgoing_buffer, buffers_elem_num*sizeof(PacketBuffer), cudaMemcpyDeviceToHost);
    CudaSafeCall( cudaMemcpy(routing_table, device_routing_table_ptr, total_elem_num*sizeof(RoutingEntry), cudaMemcpyDeviceToHost) );

    //check the results
    printBestPath(from, to, routing_table);
    // printRoutingTable(routing_table);

    CudaSafeCall( cudaFree(device_incomming_buffer_ptr) );
    CudaSafeCall( cudaFree(device_outgoing_buffer_ptr) );
    CudaSafeCall( cudaFree(device_routing_table_ptr) );
}