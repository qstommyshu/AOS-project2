#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <omp.h>
#include <math.h>

/* -------------------------------
   OpenMP Barrier Implementation
   (Sense-Reversing Centralized Barrier)
------------------------------- */

// Global variables for the OpenMP barrier (intra-process)
static int thread_count;
static int sense;

// Initialize the OpenMP barrier with the number of threads.
void gtmp_init(int num_threads) {
    thread_count = num_threads;
    sense = 0;
}

// OpenMP barrier: each thread toggles its local sense and then atomically decrements
// the thread_count. The last thread resets the count and flips the global sense.
void gtmp_barrier() {
    int local_sense;
    #pragma omp atomic read
    local_sense = sense;
    local_sense = !local_sense;
    
    if (__sync_fetch_and_sub(&thread_count, 1) == 1) {
        thread_count = omp_get_num_threads();
        #pragma omp atomic write
        sense = local_sense;
    } else {
        while (local_sense != sense);
    }
}

// Finalize the OpenMP barrier (no cleanup needed).
void gtmp_finalize() {
    // Nothing to clean up.
}

/* -------------------------------
   MPI Barrier Implementation
   (Dissemination Barrier)
------------------------------- */

// Global variables for the MPI barrier (inter-process)
static int gtmpi_size;
static int rounds;
static int gtmpi_rank;

// A simple power-of-2 function.
int pow2(int k) {
    return 1 << k;
}

// Initialize the MPI barrier: get the total number of processes and compute the rounds.
void gtmpi_init(int num_processes) {
    gtmpi_size = num_processes;
    rounds = (int) ceil(log2(gtmpi_size));
    MPI_Comm_rank(MPI_COMM_WORLD, &gtmpi_rank);
}

// MPI dissemination barrier: in each round, each process sends a message to a partner
// determined by the round number.
void gtmpi_barrier() {
    int dummy = 1;
    MPI_Status status;
    for (int k = 0; k < rounds; k++) {
        int dest = (gtmpi_rank + pow2(k)) % gtmpi_size;
        int src = (gtmpi_rank - pow2(k) + gtmpi_size) % gtmpi_size;
        MPI_Send(&dummy, 0, MPI_INT, dest, 1, MPI_COMM_WORLD);
        MPI_Recv(&dummy, 0, MPI_INT, src, 1, MPI_COMM_WORLD, &status);
    }
}

// Finalize the MPI barrier (no cleanup needed).
void gtmpi_finalize() {
    // Nothing to clean up.
}

/* -------------------------------
   Combined Barrier Implementation
   (Synchronizes threads across multiple MPI processes)
------------------------------- */

// This combined barrier first synchronizes all threads within the process using the OpenMP barrier.
// Then, thread 0 in each process performs the MPI barrier to synchronize across nodes.
// Finally, an OpenMP barrier ensures that all threads wait until the MPI barrier is complete.
void combined_barrier() {
    int tid = omp_get_thread_num();
    
    // Step 1: Local (intra-process) barrier using the custom OpenMP barrier.
    gtmp_barrier();
    
    // Step 2: Only thread 0 performs the MPI barrier across processes.
    if (tid == 0) {
        gtmpi_barrier();
    }
    
    // Step 3: All threads wait until thread 0 finishes the MPI barrier.
    #pragma omp barrier
}

/* -------------------------------
   Main Program
------------------------------- */
int main(int argc, char** argv) {
    int provided;
    // Initialize MPI with thread support.
    MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &provided);
    if (provided < MPI_THREAD_MULTIPLE) {
        fprintf(stderr, "Error: MPI does not support MPI_THREAD_MULTIPLE\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }
    
    int mpi_rank, mpi_size;
    MPI_Comm_rank(MPI_COMM_WORLD, &mpi_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &mpi_size);
    
    // Check command line arguments for number of threads per process.
    if (argc < 2) {
        if (mpi_rank == 0) {
            fprintf(stderr, "Usage: %s num_threads\n", argv[0]);
        }
        MPI_Finalize();
        exit(EXIT_FAILURE);
    }
    
    int num_threads = atoi(argv[1]);
    if (num_threads < 1) {
        if (mpi_rank == 0) {
            fprintf(stderr, "Number of threads must be at least 1\n");
        }
        MPI_Finalize();
        exit(EXIT_FAILURE);
    }
    
    // Initialize both barriers.
    gtmpi_init(mpi_size);      // MPI barrier initialization.
    gtmp_init(num_threads);    // OpenMP barrier initialization.
    
    // Example: Run the combined barrier.
    #pragma omp parallel num_threads(num_threads)
    {
        int tid = omp_get_thread_num();
        printf("Process %d, thread %d reached combined barrier\n", mpi_rank, tid);
        fflush(stdout);
        
        combined_barrier();
        
        printf("Process %d, thread %d passed combined barrier\n", mpi_rank, tid);
        fflush(stdout);
    }
    
    // Finalize both barriers.
    gtmp_finalize();
    gtmpi_finalize();
    
    MPI_Finalize();
    return 0;
}
