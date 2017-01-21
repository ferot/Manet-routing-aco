/*
 * main.cpp
 *
 *  Created on: Jan 11, 2017
 *      Author: fero
 */
#include <mpi.h>
#include <time.h>
#include <cstdlib>
#include "model/Graph.h"

using namespace std;

#define NUM_NODES 100;
#define NUM_COMMS 5;

Graph mockSimpleGraph();

typedef std::shared_ptr<Node> tNodeptr;

int main() {
    MPI_Datatype MPI_NODE, MPI_BEST;

    MPI_Init(NULL, NULL);

    // Get the number of processes
    int world_size;
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);

    // Get the rank of the process
    int rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    // Capture the starting time
	MPI_Barrier(MPI_COMM_WORLD);
    double start = 0.0, finish = 0.0;
	start = MPI_Wtime();

    srand(time(NULL)); //it has global effect

    if (!rank) {
        // @todo utworzyc Nody do przeslania do wszystkich procesow
        // Node node[NUM_NODES] = ...
        Graph graph = mockSimpleGraph();
    }
    
    // Broadcast the nodes to all processes
    // Trzeba uproscic typ node (struct skladajacy sie z typow prostych)
	MPI_Type_contiguous(2, MPI_INT, &MPI_NODE); // stuktura skladajaca sie z 2 int
	MPI_Type_commit(&MPI_NODE);
	MPI_Bcast(node, NUM_NODES, MPI_NODE, 0, MPI_COMM_WORLD);

    // Polaczyc nody i poustawiac, rozlozyc mrowki
	_linkNodes();
	ACO_Reset_ants();

/*
     for (int k=1; k < 30000; ++k) {
         graph.sendData(rand() % 8 + 1, rand() % 8 + 1);
         for(int i=0; i < rand() % 10; ++i) graph.tick();
     }

     for(int i=0; i < 20; ++i)
         graph.tick();
*/

    // Build the derived data type for communication of the best tour
	_buildBest(&best, &MPI_BEST);

    for(i=0; i<NUM_COMMS; i++) {
		for(j=0; j<NUM_TOURS*NUM_NODES; j++) {
			ACO_Step_ants();
			if(j % NUM_NODES == 0 && j != 0) {
				ACO_Update_pheromone();
				ACO_Update_best();
				ACO_Reset_ants();
			}
		}
		
		// Collect best tours from all processes
		MPI_Gather(&best, 1, MPI_BEST, all_best, 1, MPI_BEST, 0, MPI_COMM_WORLD);
		if(!rank) {
			min = 0;
			for(j=1; j<procs; j++) {
				if(all_best[j].distance < all_best[min].distance) { 
					min = j;
				}
			}
			best.distance = all_best[min].distance;

			for(j=0; j<NUM_NODES; j++) {
				best.path[j] = all_best[min].path[j];
			}
			printf("Best Distance So Far: %.15f\n", best.distance);
			fflush(stdout);
		}
		
		if(i < NUM_NODES-1) {
			// Broadcast the overall best tour to all processes
			MPI_Bcast(&best, 1, MPI_BEST, 0, MPI_COMM_WORLD);
			for(j=0; j<NUM_NODES; j++) {
				for(k=0; k<NUM_NODES; k++) {
					pheromone[j][k] = 1.0/NUM_NODES;
				}
			}
			// Highlight the overall best tour in the pheromone matrix
			for(j=0; j<NUM_NODES; j++) {
				if(j < NUM_NODES-1) {
					pheromone[best.path[j]][best.path[j+1]] += Q/best.distance;
					pheromone[best.path[j+1]][best.path[j]] = pheromone[best.path[j]][best.path[j+1]];
				}
			}
		}
	}
	
	// Capture the ending time
	MPI_Barrier(MPI_COMM_WORLD);
	finish = MPI_Wtime();
	if(!rank) {
		printf("Final Distance (%.15f): %.15f\n", finish-start, best.distance);
		fflush(stdout);
	}
	
	MPI_Type_free(&MPI_NODE);
	MPI_Type_free(&MPI_BEST);
	MPI_Finalize();

    graph.printRoutingTables();

	return 0;
}

Graph mockSimpleGraph() {
	Graph graph;

    tNodeptr n1Ptr = std::make_shared<Node>("S", 1);
    tNodeptr n2Ptr = std::make_shared<Node>("1", 2);
    tNodeptr n3Ptr = std::make_shared<Node>("2", 3);
    tNodeptr n4Ptr = std::make_shared<Node>("3", 4);
    tNodeptr n5Ptr = std::make_shared<Node>("4", 5);
    tNodeptr n6Ptr = std::make_shared<Node>("5", 6);
    tNodeptr n7Ptr = std::make_shared<Node>("6", 7);
    tNodeptr n8Ptr = std::make_shared<Node>("D", 8);

    n1Ptr->addNeighbour(n2Ptr);
    n2Ptr->addNeighbour(n1Ptr);

    n1Ptr->addNeighbour(n3Ptr);
    n3Ptr->addNeighbour(n1Ptr);

    n3Ptr->addNeighbour(n4Ptr);
    n4Ptr->addNeighbour(n3Ptr);

    n4Ptr->addNeighbour(n5Ptr);
    n4Ptr->addNeighbour(n7Ptr);
    n5Ptr->addNeighbour(n4Ptr);
    n7Ptr->addNeighbour(n4Ptr);

    n5Ptr->addNeighbour(n6Ptr);
    n6Ptr->addNeighbour(n5Ptr);

    n6Ptr->addNeighbour(n8Ptr);
    n8Ptr->addNeighbour(n6Ptr);

    n7Ptr->addNeighbour(n8Ptr);
    n8Ptr->addNeighbour(n7Ptr);

    graph.addNode(n1Ptr);
    graph.addNode(n2Ptr);
    graph.addNode(n3Ptr);
    graph.addNode(n4Ptr);
    graph.addNode(n5Ptr);
    graph.addNode(n6Ptr);
    graph.addNode(n7Ptr);
    graph.addNode(n8Ptr);

    return graph;
}

/** _buildBest
	Build an MPI derived type for communications of the best tour.
	
	@param tour
	Pointer to the place we are storing our best tour (for the current process).
	
	@param mpi_type
	Pointer to the MPI derived type we are building.
	
	@return void
*/
void _buildBest(ACO_Best_tour *tour, MPI_Datatype *mpi_type /*out*/)
{
//przyklad dla typu:
// typedef struct { 
//  double distance;
// 	int path[NUM_NODES];
// } ACO_Best_tour;
	int block_lengths[2];
	MPI_Aint displacements[3];
	MPI_Datatype typelist[3];
	MPI_Aint start_address;
	MPI_Aint address;
	
	block_lengths[0] = 1;
	block_lengths[1] = NUM_NODES;
	
	typelist[0] = MPI_DOUBLE;
	typelist[1] = MPI_INT;
	
	displacements[0] = 0;
	
	MPI_Address(&(tour->distance), &start_address);
	MPI_Address(tour->path, &address);
	displacements[1] = address - start_address;
	
	MPI_Type_struct(2, block_lengths, displacements, typelist, mpi_type);
	MPI_Type_commit(mpi_type);
}