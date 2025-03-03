#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include "gtmpi.h"

int main(int argc, char** argv)
{
  int num_processes, num_rounds = 1;

  MPI_Init(&argc, &argv);
  
  if (argc < 2){
    fprintf(stderr, "Usage: ./harness [NUM_PROCS]\n");
    exit(EXIT_FAILURE);
  }

  num_processes = strtol(argv[1], NULL, 10);

  gtmpi_init(num_processes);
  
  int k;
  for(k = 0; k < num_rounds; k++){
    int tid;
    MPI_Comm_rank(MPI_COMM_WORLD, &tid);

    printf("thread %d reaches barrier\n", tid);

    gtmpi_barrier();

    printf("thread %d passes the barrier\n", tid);
  }

  gtmpi_finalize();  

  MPI_Finalize();

  return 0;
}
