# c-synchronization
Playing around with synchronization, race conditions, atomics and memory barriers in C

# What is a Data Race
According to the C language convention a data race is:
- When there is shared data between two threads or processes
- And at least one of those threads/processes writes to the data while others read

A data race may cause undefined behavior in stores and loads on that shared data. It may
also cause the value of that variable to be unpredictable.

A data race can happen mainly due to the following reasons:
- Compiler optimizations and code re-ordering
- Concurrency
- CPU speculative and out of order executation

According to the C language specification a data race is removed by either:
- Using atomic loads and stores on the shared data
- Establishing happens-before relationship between different load and store sites on the shared data

Note that while atomic operations on the shared data, does remove data race, it can still
result in different outcome (value) for the shared data depending on the compiler and CPU
peculiarities.

Now let's illustrate some of these concepts via a few examples. 

# Data Race on a Shared Counter
In the first example we consider a shared global counter variable which is independently updated
by two threads, ```thread_inc``` and ```thread_dec```, where the former threads increments the counter
while the latter decrements it. This is illustrated in [race-condition.c](./race-condition.c):
```
int counter = 0;

void *thread_inc(void *buf) {
    int i = 0;

    for (i = 0; i < 10000000; i++) {
        counter ++;
        asm volatile ("" ::: "memory");
    }
}

void *thread_dec(void *buf) {
    int i = 0;

    for (i = 0; i < 10000000; i++) {
        counter --;
        asm volatile ("" ::: "memory");
    }
}
```

Let's compile and build the program and look at the generated assembly:
```
gcc -g3 -O2 race-condition.c -o race-condition -lpthread
```
I am using -g3 to add full debug info and -O2 to have compiler optimizations. Now we can inspect the assembly along with the source code:
```
objdump -Sd race-condition | less
```

Let'ls focus on the incrementing thread, ```thread_inc()``` function for example:
```
000000000001410 <thread_inc>:
void *thread_inc(void *buf) {
    1410:       f3 0f 1e fa             endbr64 
    1414:       b8 80 96 98 00          mov    $0x989680,%eax
    1419:       0f 1f 80 00 00 00 00    nopl   0x0(%rax)
        counter ++;
    1420:       83 05 ed 2b 00 00 01    addl   $0x1,0x2bed(%rip)        # 4014 <counter>
    for (i = 0; i < 10000000; i++) {
    1427:       83 e8 01                sub    $0x1,%eax
    142a:       75 f4                   jne    1420 <thread_inc+0x10>
}
    142c:       c3                      retq   
    0142d:       0f 1f 00                nopl   (%rax)
```
The important line is on relative file address ```1420``` where the counter memory location is being added by one:
```
    1420:       83 05 ed 2b 00 00 01    addl   $0x1,0x2bed(%rip)        # 4014 <counter>
```
As you can see this is just one assembly instruction that directly adds ``0x1`` to the memory address holding ``counter``.

> Important side note: the inline assembly line in the C code is merely there to prevent the C compiler from optimizing out the counter update and replacing it with ``counter = 10000000``.
```
        asm volatile ("" ::: "memory");
```
This is basically a ``nop`` assembly instruction which may be removed by the C compiler, hence we use the keyword ``volatile`` followed by ``asm`` to tell the C compiler that this instruction should be observed. Also the ``memory`` at the last part is what we call the clobber of the inline assembly which basically tells the compiler that we expect the state of memory to have changed after this ``nop`` instruction. The end result is that the C compiler would not perform any optimizations including re-ordering of the counter update and would respect it by generating the ``addl`` instruction per loop iteration.

If we run this program we expect the eventual value of the counter to be zero as it is ultimately incremented and decremented by the same number of times. However, multiple runs show that each time the counter rests at a different random final result:
```
$ ./race-condition 
Final counter value is -693983
$ ./race-condition 
Final counter value is -2199129
$ ./race-condition 
Final counter value is -4578855
$ ./race-condition 
Final counter value is 3008624
$ ./race-condition 
Final counter value is -4332533
```
This is cause by having a data race on ``counter`` as defined by C-language convention: We have two threads both updating a shared data, i.e., ``counter``. Surprisingly, the race condition happens despite the counter update happening during a single assembly instruction ``addl``, ``subl``. 
> Assembly instructions are not necessarily executed atomically unless there are explicit or implicit hardware guarantees.
 
In fact, what could happen is that ``counter`` is updated on the private L1/L2 cache of one core running one thread, while the other core running the other thread is yet to be updated via cache coherence protocol. 
> Even if an assembly instruction is executed atomically on the CPU it is being executed, it may not look atomic from the perspective of other CPUs.

Hence, the other thread will get the wrong value of ``counter`` to start with. Whichever thread wins the ``store`` race is the one that would determine the final result of ``counter`` in this instance of the **data race**. This winning is determined not only by chronological order of execution but also by CPU speculative execution, store and load buffer implementation, and cache coherenece protocol, among many other things.

The race condition can be removed as specified by C language convention by using atomic operations for the counter update, as one option:
```
#include <stdatomic.h>
...
atomic_int counter = 0;
atomic_fetch_add(&counter, 1);
```
There are other ways to do this but which would be compiler dependent such as the sync_* directives for GCC. However, using the atmoic types and operations from ``stdatomic.h`` are the portable option which works across compilers and CPU architectures. The new ``thread_inc`` would now look like: 
```
void *thread_inc(void *buf) {
    int i = 0;

    for (i = 0; i < 10000000; i++) {
        atomic_fetch_add(&counter,1);
        asm volatile ("" ::: "memory");
    }
}
```
