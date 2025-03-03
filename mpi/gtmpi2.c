#include <stdlib.h>
#include <mpi.h>
#include <stdio.h>
#include "gtmpi.h"

// Tree barrier from the lecture video, not exactly the same as the MCS paper one. The idea is very similar though.

static int gtmpi_rank;
static int gtmpi_size;

void gtmpi_init(int num_processes) {
    MPI_Comm_rank(MPI_COMM_WORLD, &gtmpi_rank);
    // MPI_Comm_size(MPI_COMM_WORLD, &gtmpi_size);
    gtmpi_size = num_processes;
    if (num_processes != gtmpi_size) {
        fprintf(stderr, "Warning: Provided num_processes (%d) does not match MPI_Comm_size (%d)!\n", 
                num_processes, gtmpi_size);
    }
}

void gtmpi_barrier() {
    int dummy = 1;
    MPI_Status status;
    
    // Compute the parent and left/right children based on a binary tree structure.
    int parent = (gtmpi_rank - 1) / 2;
    int left_child = 2 * gtmpi_rank + 1;
    int right_child = 2 * gtmpi_rank + 2;

    // Arrival Phase: If a left child exists, receive an arrival message from it.
    if (left_child < gtmpi_size) {
        MPI_Recv(&dummy, 1, MPI_INT, left_child, 0, MPI_COMM_WORLD, &status);
    }
    // If a right child exists, receive an arrival message from it.
    if (right_child < gtmpi_size) {
        MPI_Recv(&dummy, 1, MPI_INT, right_child, 0, MPI_COMM_WORLD, &status);
    }
    // For non-root processes, send an arrival message to the parent.
    if (gtmpi_rank != 0) {
        MPI_Send(&dummy, 1, MPI_INT, parent, 0, MPI_COMM_WORLD);
        // Then wait for the wakeup message from the parent.
        MPI_Recv(&dummy, 1, MPI_INT, parent, 1, MPI_COMM_WORLD, &status);
    }
    // Wakeup Phase: Send the wakeup message to any existing children to notify them to exit the barrier.
    if (left_child < gtmpi_size) {
        MPI_Send(&dummy, 1, MPI_INT, left_child, 1, MPI_COMM_WORLD);
    }
    if (right_child < gtmpi_size) {
        MPI_Send(&dummy, 1, MPI_INT, right_child, 1, MPI_COMM_WORLD);
    }
}

void gtmpi_finalize() {
}
