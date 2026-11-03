/* 

build:

gcc -g3 -o petersons-alg petersons-alg.c -lpthread

run it as follows to see the effect 
of the race condition:

for i in {1..10}; do ./petersons-alg ; done;

inspect the assembly:

objdump -Sd petersons-alg | less
*/
#define _GNU_SOURCE
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdatomic.h>

int counter = 0;
atomic_int flag[2] = {0, 0};
atomic_int turn = 0;

// P0
void *thread_inc(void *buf) {
    int i = 0;
    int tmp;

    for (i = 0; i < 10000000; i++) {
        atomic_store_explicit(&flag[0], 1, memory_order_release);
        atomic_store_explicit(&turn, 1, memory_order_release);
        while (atomic_load_explicit(&flag[1], memory_order_acquire) == 1 && \
                atomic_load_explicit(&turn, memory_order_acquire) == 1)
            ;
        counter ++;
        atomic_store_explicit(&flag[0], 0, memory_order_release);
    }
}

// P1
void *thread_dec(void *buf) {
    int i = 0;
    int tmp;

    for (i = 0; i < 10000000; i++) {
        atomic_store_explicit(&flag[1], 1, memory_order_release);
        atomic_store_explicit(&turn, 0, memory_order_release);
        while (atomic_load_explicit(&flag[0], memory_order_acquire) == 1 && \
                atomic_load_explicit(&turn, memory_order_acquire) == 0)
            ;
        counter --;
        atomic_store_explicit(&flag[1], 0, memory_order_release);
    }
}

int main() {
    pthread_attr_t attr;
    pthread_t t1,t2;
    int s = 0;
    void *res;
    cpu_set_t c;   

    CPU_ZERO(&c);
    CPU_SET(3, &c);

    s = pthread_attr_init(&attr);
    if (s)
        perror("pthread_attr_init");

    //pthread_attr_setaffinity_np(&attr, sizeof(c), &c);

    s = pthread_create(&t1, &attr, thread_inc, NULL);
    if (s)
        perror("pthread_create for t1");
    s = pthread_create(&t2, &attr, thread_dec, NULL);
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

    printf("Final counter value is %d\n", counter);

    return 0;
}