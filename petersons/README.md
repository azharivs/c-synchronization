# Peterson's Mutual Exclusion Algorithm
In this part I will walk you step by step into the Peterson's mutual exclusion solution and why it fails to work in today's
modern CPU architectures. At the end you will realize we need not use the Peterson's algorithm to secure the ``counter`` update
critical section. All we need to do is to use an atomic counter. Nevertheless, studying the Peterson's algorithm example is
very instructive in learning different aspects of synchronization and race conditions.

## Plain Peterson's Algorithm
In this version we will use normal variables for the flag and turn shared data of the Peterson't algorithm.
It should supposedly remove the race condition on the counter update. 
```
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
```
However, we still get random non-zero values for ``counter`` indicating that we have a race! Here is the assembly for the ``thread_inc``
function as an example:
```
000000000001410 <thread_inc>:
void *thread_inc(void *buf) {
    1410:       f3 0f 1e fa             endbr64 
    1414:       ba 80 96 98 00          mov    $0x989680,%edx
    1419:       0f 1f 80 00 00 00 00    nopl   0x0(%rax)
        flag[0] = 1;
    1420:       c7 05 f6 2b 00 00 01    movl   $0x1,0x2bf6(%rip)        # 4020 <flag>
    1427:       00 00 00 
        turn = 1;
    142a:       c7 05 e4 2b 00 00 01    movl   $0x1,0x2be4(%rip)        # 4018 <turn>
    1431:       00 00 00 
        while (flag[1] && turn == 1)
    1434:       eb 15                   jmp    144b <thread_inc+0x3b>
    1436:       66 2e 0f 1f 84 00 00    nopw   %cs:0x0(%rax,%rax,1)
    143d:       00 00 00 
    1440:       8b 05 d2 2b 00 00       mov    0x2bd2(%rip),%eax        # 4018 <turn>
    1446:       83 f8 01                cmp    $0x1,%eax
    1449:       75 0a                   jne    1455 <thread_inc+0x45>
    144b:       8b 05 d3 2b 00 00       mov    0x2bd3(%rip),%eax        # 4024 <flag+0x4>
    1451:       85 c0                   test   %eax,%eax
    1453:       75 eb                   jne    1440 <thread_inc+0x30>
        counter ++;
    1455:       83 05 cc 2b 00 00 01    addl   $0x1,0x2bcc(%rip)        # 4028 <counter>
        flag[0] = 0;
    145c:       c7 05 ba 2b 00 00 00    movl   $0x0,0x2bba(%rip)        # 4020 <flag>
    1463:       00 00 00 
    for (i = 0; i < 10000000; i++) {
    1466:       83 ea 01                sub    $0x1,%edx
    1469:       75 b5                   jne    1420 <thread_inc+0x10>
}
    146b:       c3                      retq   
    146c:       0f 1f 40 00             nopl   0x0(%rax)
```
The relevant lines are listed below. The first three load turn into a register and check if it is one, while the
next three lines similarly check if flag[1] is equal to one. Note that the load is done in one instruction but
as we saw with [race-condition](race/race-condition.c), the load may not be seen by other cores as atomic.
```
    1440:       8b 05 d2 2b 00 00       mov    0x2bd2(%rip),%eax        # 4018 <turn>
    1446:       83 f8 01                cmp    $0x1,%eax
    1449:       75 0a                   jne    1455 <thread_inc+0x45>

    144b:       8b 05 d3 2b 00 00       mov    0x2bd3(%rip),%eax        # 4024 <flag+0x4>
    1451:       85 c0                   test   %eax,%eax
    1453:       75 eb                   jne    1440 <thread_inc+0x30>

```
That is to say, when ``thread_dec`` is doing a store on its ``flag`` and ``turn`` it may not be see as atomic
to the core executing ``atomic_inc``, hence breaking Peterson's proper ordering of the algorithm. Moreover,
the order with which ``flag`` and ``turn`` are set by the same core may be switched as there are no apparent data dependency
among the two assignments. Again, this can break Peterson's algorithm.

## Making the Stores Atomic
Next we use atomic store operations as in [petersons-atomic-store.c](petersons-atomic-store.c).
```
void *thread_inc(void *buf) {
    int i = 0;
    int tmp;

    for (i = 0; i < 10000000; i++) {
        __sync_lock_test_and_set(&flag[0], 1);
        __sync_lock_test_and_set(&turn, 1);
        while (flag[1] && turn == 1)
            ;
        counter ++;
        __sync_lock_release(&flag[0]);
    }
}
```
The generated assembly with -O2 optimization is as follows:
```

```
