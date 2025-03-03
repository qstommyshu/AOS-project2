#include <stdlib.h>
#include <mpi.h>
#include <math.h>
#include "gtmpi.h"

// dissemination barrier from the lecture video, not exactly the same as the MCS paper one.

static int thread_count;
static int rounds;

int pow2(int k) {
	return 1 << k;
}

void gtmpi_init(int num_threads){
	thread_count = num_threads;
    rounds = (int) ceil(log2(thread_count));
}

void gtmpi_barrier(){
	int i;
	MPI_Status status;
	MPI_Comm_rank(MPI_COMM_WORLD, &i);

	int k;
	for (k = 0; k < rounds; k++) {
        // dest is the node current node sends info to
		int dest = (i + pow2(k)) % thread_count;
        // src is the node which sends info to current node
		int src = (i - pow2(k) + thread_count) % thread_count;

        // Sending an empty message NULL is enough in this case
		MPI_Send(NULL, 0, MPI_INT, dest, 1, MPI_COMM_WORLD);
        MPI_Recv(NULL, 0, MPI_INT, src, 1, MPI_COMM_WORLD, &status);
	}
}

void gtmpi_finalize(){
}