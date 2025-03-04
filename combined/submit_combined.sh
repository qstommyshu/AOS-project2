#!/bin/bash

# Directory to store the sbatch files
OUTPUT_DIR="./sbatch"
mkdir -p "$OUTPUT_DIR"

# Number of jobs to create and submit
MPI_PROCS_LIST=(2 3 4 5 6 7 8)  # Number of MPI processes
OMP_THREADS_LIST=(2 3 4 5 6 7 8 9 10 12)  # Number of OpenMP threads per process
SCRIPT_TO_RUN="combined"      
SOURCE_TO_COMPILE="combined.c"
NUM_ROUNDS=1000000

# Loop over different Node/CPU counts
for MPI_PROCS in "${MPI_PROCS_LIST[@]}"; do
  for OMP_THREADS in "${OMP_THREADS_LIST[@]}"; do
    # New sbatch file name
    NEW_SBATCH="$OUTPUT_DIR/$SCRIPT_TO_RUN-PROCESS${MPI_PROCS}-THREADS${OMP_THREADS}.sbatch"
    # New output file name
    OUTPUT_FILE="$OUTPUT_DIR/$SCRIPT_TO_RUN-PROCESS${MPI_PROCS}-THREADS${OMP_THREADS}.out"

    cat > "$NEW_SBATCH" <<EOL
#!/bin/bash

#SBATCH -J cs6210-proj2-combined
#SBATCH -N ${MPI_PROCS}  # Number of MPI processes
#SBATCH --ntasks-per-node=1
#SBATCH --mem-per-cpu=1G
#SBATCH -t 10
#SBATCH -q coc-ice
#SBATCH --cpus-per-task=$((OMP_THREADS + 1))
#SBATCH -o $OUTPUT_FILE

echo "Started on \$(/bin/hostname)"

module load gcc/12.3.0 mvapich2/2.3.7-1

if [[ ! -f "$SCRIPT_TO_RUN" ]]; then
    echo "Executable '$SCRIPT_TO_RUN' not found, compiling..."
    make $SCRIPT_TO_RUN
fi

export OMP_NUM_THREADS=$OMP_THREADS

# Run the script
srun ./$SCRIPT_TO_RUN $NUM_ROUNDS $OMP_THREADS
EOL

  sleep 5  # Delay between job submissions
  # Submit the new sbatch file
  sbatch "$NEW_SBATCH"
  done
done

echo "All jobs created and submitted haha."