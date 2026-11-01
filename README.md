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


