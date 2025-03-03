#include <stdlib.h>
#include <mpi.h>
#include <math.h>
#include "gtmpi.h"

// dissemination barrier from the lecture video, not exactly the same as the MCS paper one.

static int gtmpi_size;
static int rounds;
static int gtmpi_rank;

int pow2(int k) {
	return 1 << k;
}

void gtmpi_init(int num_threads){
	gtmpi_size = num_threads;
    rounds = (int) ceil(log2(gtmpi_size));
    MPI_Comm_rank(MPI_COMM_WORLD, &gtmpi_rank);
}

void gtmpi_barrier(){
    int dummy = 1;
	MPI_Status status;

	for (int k = 0; k < rounds; k++) {
        // dest is the node current node sends info to
		int dest = (gtmpi_rank + pow2(k)) % gtmpi_size;
        // src is the node which sends info to current node
		int src = (gtmpi_rank - pow2(k) + gtmpi_size) % gtmpi_size;

        // Sending an empty message NULL is enough in this case
		MPI_Send(&dummy, 0, MPI_INT, dest, 1, MPI_COMM_WORLD);
        MPI_Recv(&dummy, 0, MPI_INT, src, 1, MPI_COMM_WORLD, &status);
	}
}

void gtmpi_finalize(){
}