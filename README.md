PIAlgoChud - Multithreaded Chudnovsky Algorithm for Calculating PI
===
## Source Project (komasaru's single thread implementation)
- Single thread source of chudnovsky with binary splitting
    - https://gist.github.com/komasaru/68f209118edbac0700da

## Feature
- Multithread, Affinity, Blocking-Queue, Master-Worker Architect
- 3.8x speedup in 8 cores with 100,000,000 digits compares to single core Chudnovsky, completes in 32,904 ms
- 2.8x speedup in 4 cores with 100,000,000 digits compares to single core Chudnovsky, completes in 44,896 ms
- 1.8x speedup in 2 cores with 100,000,000 digits compares to single core Chudnovsky, completes in 69,882 ms
- single core Chudnovsky completes in 127,179 ms


## Some Related References
#### Algorithm to Calculate PI
- An response from the world record holder of the most digits of pi
    - https://stackoverflow.com/questions/14283270/how-do-i-determine-whether-my-calculation-of-pi-is-accurate
- Sample implementation and demonstration
    - https://www.craig-wood.com/nick/articles/pi-chudnovsky/
#### Verifier
- PI world record with sample data for verification
    - http://www.numberworld.org/digits/Pi/

## Implementation Details
- 3 versions of multithread implementation
- 3 parts of multithread stage:
    - Part 1. binary splitting into suitable number of batch (this number must be power of 2)
        - can use all cores
    - Part 2. master distribute the sub-task(multiplication & addition) of merge to workers, then retrieve results from them
        - can use all cores
    - Part 3. merge the final result
        - can use only 2 cores

## Prerequisition
- C++17
    - g++
- Gnu MP Library for C++
    - libgmp-dev
- Boost C++ Library
    - libboost-all-dev

## Build
```
// normal build
# make
// -O3
# make optim
// debug symbol
# make debug
```

## Test
```
# make test
```

## Usage
```
usage: {exe} -p {digits} [-w {workers}] [-v {version}] [(-s|-m|-sm)] [(-n)]

   -p: specify the precision of PI.
   -w: specify the number of worker.
   -v: specify the verion of multithread implementation. Currently 1, 2, 3 is available, and default is 2.
   -s: using single thread mode to calculate PI.
   -m: using multi thread mode to calculate PI. Default.
   -sm: using both single thread and multi thread mode to calculate PI.
   -n: do not output.
   -h: print this message.
```

## Tools
- Valgrind (Memory)
    - valgrind
- Perf (Profiler)
    - linux-tools-$(uname -r)
- Hotspot (FlameGraph & GUI)
    - hotspot
- Gdb (Debugger)
    - gdb

## Test Environment
- OS: Ubuntu 20.04 Focal (5.13.0-40-generic)
- CPU: 8 Cores
- Mem: 16GB
- Compiler: g++ (Ubuntu 9.4.0-1ubuntu1~20.04.1) 9.4.0
