#include <omp.h>
#include "gtmp.h"
#include <stdio.h>
#include <stdlib.h>

// MCS Paper: A software combining tree barrier with optimized wakeup

/*
    type node = record
        k : integer //fan in of this node
	count : integer // initialized to k
	locksense : Boolean // initially false
	parent : ^node // pointer to parent node; nil if root

	shared nodes : array [0..P-1] of node
	    //each element of nodes allocated in a different memory module or cache line

	processor private sense : Boolean := true
	processor private mynode : ^node // my group's leaf in the combining tree

	procedure combining_barrier
	    combining_barrier_aux (mynode) // join the barrier
	    sense := not sense             // for next barrier


	procedure combining_barrier_aux (nodepointer : ^node)
	    with nodepointer^ do
	        if fetch_and_decrement (&count) = 1 // last one to reach this node
		    if parent != nil
		        combining_barrier_aux (parent)
		    count := k // prepare for next barrier
		    locksense := not locksense // release waiting processors
		repeat until locksense = sense
*/

#define MAX_FAN_IN 2

typedef struct node {
    int k;
    int count;
    int locksense;
    struct node *parent;
} node;

static node **nodes;

// thread local variables
static int sense = 0;
static node *mynode;

void gtmp_init(int num_threads) {
    nodes = (node **)malloc(num_threads * sizeof(node *));
    // allocate memory for all nodes
    for (int i = 0; i < num_threads; i++) {
        nodes[i] = (node *)malloc(sizeof(node));
    }
    // node initialization
    for (int i = 0; i < num_threads; i++) {
        int child_count = 0;
        int first_child = i * MAX_FAN_IN + 1;
        for (int j = first_child; j < first_child + MAX_FAN_IN && j < num_threads; j++) {
            child_count++;
        }
        // k is the fan in of a node plus its own arrival
        nodes[i]->k = child_count + 1;
        nodes[i]->count = nodes[i]->k;
        nodes[i]->locksense = 0;
        nodes[i]->parent = (i == 0) ? NULL : nodes[(i - 1) / MAX_FAN_IN];
    }

    // debug
    for (int i = 0; i < num_threads; i++) {
        printf("node %d's parent is %d, k = %d, count = %d\n", 
               i, 
               (nodes[i]->parent ? (i - 1) / MAX_FAN_IN : -1), 
               nodes[i]->k, 
               nodes[i]->count);
    }
}


static void combining_barrier_aux(node *cur, int sense) {
    // int tid = omp_get_thread_num();
    // printf("thread %d's node count is %d\n", tid, cur->count);
    // last arrival flip the sense variable and count
    if (__sync_fetch_and_sub(&cur->count, 1) == 1) {
        if (cur->parent) combining_barrier_aux(cur->parent, sense);
        cur->count = cur->k;
        cur->locksense = !cur->locksense;
        // printf("thread %d flip the lock sense------\n", tid);
    } else {
        while (cur->locksense != sense);
    }
}

void gtmp_barrier() {
    int tid = omp_get_thread_num();
    mynode = nodes[tid];
    sense = !mynode->locksense;
    combining_barrier_aux(mynode, sense);
}

void gtmp_finalize() {
    for (int i = 0; i < omp_get_num_threads(); i++) free(nodes[i]);
    free(nodes);
}