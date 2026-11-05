#define _GNU_SOURCE
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdatomic.h>

#ifndef MEM_ORDER_STORE
#define MEM_ORDER_STORE memory_order_relaxed
#endif

#ifndef MEM_ORDER_LOAD
#define MEM_ORDER_LOAD memory_order_relaxed
#endif

int data = 0;
int i = 0;
atomic_int ready = 0;

// P0
void *thread_0(void *buf) {
    
    data = 100;
    atomic_store_explicit(&ready, 1, MEM_ORDER_STORE);
}

// P1
void *thread_1(void *buf) {

    while(atomic_load_explicit(&ready, MEM_ORDER_LOAD) != 1)
        i++;
}

int main() {
    pthread_attr_t attr;
    pthread_t t1,t2;
    int s = 0;
    void *res;

    s = pthread_attr_init(&attr);
    if (s)
        perror("pthread_attr_init");

    s = pthread_create(&t1, &attr, thread_1, NULL);
    if (s)
        perror("pthread_create for t1");
    s = pthread_create(&t2, &attr, thread_0, NULL);
    if (s)
        perror("pthread_create for t2");

    s = pthread_attr_destroy(&attr);
    if (s)
        perror("pthread_attr_destroy");
    
    s = pthread_join(t1, &res);
    if (s)
        perror("pthread_join for t1");
    s = pthread_join(t2, &res);
    if (s)
        perror("pthread_join for t2");

    printf("data = %d, iterations = %d\n", data, i);

    return 0;
}