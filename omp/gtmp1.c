#include <omp.h>
#include "gtmp.h"
#include <stdio.h>

// MCS Paper: A sense-reversing centralized barrier

/*
    shared count : integer := P
    shared sense : Boolean := true
    processor private local_sense : Boolean := true

    procedure central_barrier
        local_sense := not local_sense // each processor toggles its own sense
        if fetch_and_decrement (&count) = 1
            count := P
            sense := local_sense // last processor toggles global sense
        else
           repeat until sense = local_sense
*/

static int thread_count;
static int sense;

void gtmp_init(int num_threads){
    thread_count = num_threads;
    sense = 0;
}

void gtmp_barrier(){
    int local_sense;

    // int cur_count = omp_get_thread_num();

    #pragma omp atomic read
    local_sense = sense;
    local_sense = !local_sense;

    // #pragma omp atomic
    // thread_count--;

    if (__sync_fetch_and_sub(&thread_count, 1) == 1) {
        // printf("thread %d set count back to N-------\n", cur_count);
        thread_count = omp_get_num_threads();

        #pragma omp atomic write
        sense = local_sense;
    } else {
        // printf("thread %d spining...\n", cur_count);
        while (local_sense != sense);
    }

}

void gtmp_finalize(){

}

