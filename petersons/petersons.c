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

int counter = 0;
volatile int flag[2] = {0, 0};
volatile int turn = 0;

// P0
void *thread_inc(void *buf) {
    int i = 0;

    for (i = 0; i < 10000000; i++) {
        flag[0] = 1;
        turn = 1;
        while (flag[1] && turn == 1)
            ;
        counter ++;
        asm volatile ("" ::: "memory");
        flag[0] = 0;
    }
}

// P1
void *thread_dec(void *buf) {
    int i = 0;

    for (i = 0; i < 10000000; i++) {
        flag[1] = 1;
        turn = 0;
        while (flag[0] && turn == 0)
            ;
        counter --;
        asm volatile ("" ::: "memory");
        flag[1] = 0;
    }
}

int main(int argc, char **argv) {
    pthread_attr_t attr;
    pthread_t t1,t2;
    int s = 0;
    void *res;
    cpu_set_t c;   

    s = pthread_attr_init(&attr);
    if (s)
        perror("pthread_attr_init");

    if (argc == 2){
        int cpu = atoi(argv[1]);
        CPU_ZERO(&c);
        CPU_SET(cpu, &c);
        pthread_attr_setaffinity_np(&attr, sizeof(c), &c);
    }

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