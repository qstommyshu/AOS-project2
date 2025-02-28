// #include <omp.h>
// #include "gtmp.h"

// void gtmp_init(int num_threads){

// }

// void gtmp_barrier(){

// }

// void gtmp_finalize(){

// }

#include <omp.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#define MAX_FAN_IN 2

typedef struct node {
    int k;
    int count;
    int locksense;
    struct node *parent;
} node;

static node **nodes;

// 每个线程的私有变量
static __thread int sense = 0;
static __thread node *mynode;

// void gtmp_init(int num_threads) {
//     nodes = (node **)malloc(num_threads * sizeof(node *));

//     // // root init
//     // nodes[0] = (node *)malloc(sizeof(node));
//     // nodes[0]->k = MAX_FAN_IN;
//     // nodes[0]->count = nodes[0]->k; // initialize to k
//     // nodes[0]->locksense = 0;
//     // nodes[0]->parent = NULL;

//     // 为每个线程创建叶子节点
//     for (int i = 0; i < num_threads; i++) {
//         nodes[i] = (node *)malloc(sizeof(node));
//         // if num_threads is even, then all nodes have two children, if not, the last child has one
//         nodes[i]->k = (num_threads % MAX_FAN_IN == 0 || i < num_threads - 1) ? MAX_FAN_IN : i % MAX_FAN_IN; 
//         nodes[i]->count = nodes[i]->k;
//         nodes[i]->locksense = 0;
//         nodes[i]->parent = nodes[(i - 1)/MAX_FAN_IN]; // its parent in the tree
//         // printf("num_thread is %d, value is %d, %d\n", num_threads, num_threads % MAX_FAN_IN == 0, i < num_threads - 1);
//     }

//     // root does not have parent
//     nodes[0]->parent = NULL;

//     for (int i = 0; i < num_threads; i++) {
//         printf("node %d's parent is %d\n", i, (i - 1)/MAX_FAN_IN);
//         // printf("node %d's count is %d\n", i, nodes[i]->count);
//     }
// }

void gtmp_init(int num_threads) {
    nodes = (node **)malloc(num_threads * sizeof(node *));
    // 先为所有节点分配内存
    for (int i = 0; i < num_threads; i++) {
        nodes[i] = (node *)malloc(sizeof(node));
    }
    // 为每个节点计算孩子数量，并初始化 k 和 count
    for (int i = 0; i < num_threads; i++) {
        int child_count = 0;
        // 计算在以 i 为根的子树中有多少个孩子
        int first_child = i * MAX_FAN_IN + 1;
        for (int j = first_child; j < first_child + MAX_FAN_IN && j < num_threads; j++) {
            child_count++;
        }
        // k 等于本节点（1）加上所有孩子的数量
        nodes[i]->k = child_count + 1;
        nodes[i]->count = nodes[i]->k;
        nodes[i]->locksense = 0;
        // 设置父节点：根节点的父节点为 NULL
        nodes[i]->parent = (i == 0) ? NULL : nodes[(i - 1) / MAX_FAN_IN];
    }

    // 打印每个节点的信息
    for (int i = 0; i < num_threads; i++) {
        printf("node %d's parent is %d, k = %d, count = %d\n", 
               i, 
               (nodes[i]->parent ? (i - 1) / MAX_FAN_IN : -1), 
               nodes[i]->k, 
               nodes[i]->count);
    }
}


static void combining_barrier_aux(node *cur, int sense) {
    int tid = omp_get_thread_num();
    printf("thread %d's node count is %d\n", tid, cur->count);
    if (__sync_fetch_and_sub(&cur->count, 1) == 1) {  // 当前节点最后一个到达者
        if (cur->parent) combining_barrier_aux(cur->parent, sense);
        cur->count = cur->k;        // 重置计数器
        cur->locksense = !cur->locksense; // 翻转释放锁
        printf("thread %d flip the lock sense------\n", tid);
    } else {  // 非最后一个到达者
        while (cur->locksense != sense) {
            printf("thread %d count is %d\n", tid, cur->count);
        }; // 自旋等待
    }
}

void gtmp_barrier() {
    int tid = omp_get_thread_num();
    mynode = nodes[tid];
    sense = !mynode->locksense;  // 为下一次屏障准备
    combining_barrier_aux(mynode, sense);
}

void gtmp_finalize() {
    for (int i = 0; i < omp_get_num_threads(); i++) free(nodes[i]);
    free(nodes);
}