# Atomic Variables and Operations
Atomic operations perform their operation, whether it be a load/store/add on the relevant memory location in one shot. This means that no other running thread on another core can observe an atomic operation in an undefined state in the middle.
> An atomic operation is observed by other cores/threads as either not acomplished or fully accomplished with its results visible. There is no in between.
> Note, however, that an atomic operation does not have to be observable by all cores __at the same time__. Each core may observe the atomic operation at a slightly different instance.

Atomic data types in C are data types on which atomic operations can be perfromed. These as well as many atomic operations are provided in the standard C shared library under ``stdatomic.h`` header file. It is advised that only atomic data types be used with atomic operations. Nevertheless, there are also GNU GCC compiler specific atomic directives which are not part of the C language but specific to the GCC compiler. These are GCC directives such as the ``__sync_lock_*`` which operate on basic C data types, however, they are less portable.

There are different C ordering guarantees provided by ``stdatomic.h``:
```
memory_order-relaxed
memory_order_consume
memory_order_release/memory_order_acquire
memory_order_acq_rel
memory_order_seq_cst
```
Let's start with a simple program [store-load.c](./store-load.c) that plays with atomic store followed by a load and try to understand what the differenet ordering guarantees in C actually provide.

```
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
```
Here we have a very basic writer-reader problem where ``thread_0`` stores the value zero into an ordinary ``int data`` which is initially zero and ``thread_1`` is supposed to then read it.
However, to enforce order, an atomic variable ``atomic_int ready`` is used to signal the writing of data by being set to 1 atomically by the writer thread. The reader thread, on the other hand, continuously
reads ``ready`` in a while loop and exits when it finds ``ready=1``. 

There are two main concepts at play here: order and atomicity. We need order to preserve the sequence of storing data to loading it and we need atomicity to ensure that the __shared__ variable ``ready`` is updated and read consistently by the two threads and not in the middle of an update. To do this it suffices to use the weakest form of C memory ordering, ``memory_order_relaxed``. 
