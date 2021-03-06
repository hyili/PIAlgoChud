PIAlgoChud - Multithreaded Chudnovsky Algorithm for Calculating PI
===
## Related Source Project
Chudnovsky is one of the fastest Algorithm for PI Calculation.
PIAlgoChud project is an upgrade of the original single thread implementation (by komasaru).
- Single thread source of chudnovsky with binary splitting
    - https://gist.github.com/komasaru/68f209118edbac0700da
- Algorithm implementation and demonstration
    - https://www.craig-wood.com/nick/articles/pi-chudnovsky/

## Feature
- Multithread, Affinity, Blocking-Queue, Master-Worker Architect, and Shared-Memory
- 4.00x speedup in 8 cores with 100,000,000 digits compares to single core Chudnovsky, completes in 30,395 ms
- 3.00x speedup in 4 cores with 100,000,000 digits compares to single core Chudnovsky, completes in 40,218 ms
- 1.85x speedup in 2 cores with 100,000,000 digits compares to single core Chudnovsky, completes in 65,307 ms
- Single core Chudnovsky completes in 120,450 ms

## TODO
Since we use GMP as our big number library, it cannot support multithreaded multiplication. This would be the current limitation of further improving CPU utilization. I've search for other multithreaded big number library for a period, but with no luck. If possible in the future, I'll try to implement a big number library that supports multithread to solve the bottleneck.

## Prerequisition
- C++17
    - g++
- Gnu MP Library for C++
    - libgmp-dev
- Boost C++ Library
    - libboost-all-dev

## Build
cd into the root of PIAlgoChud/
```
// normal build
# make
// -O3
# make optim
// debug symbol
# make debug
```

## Test
To test build, run
```
# make test
```

## Usage
```
usage: {exe} -p {digits} [-w {workers}] [-v {version}] [(-s|-m|-sm)] [(-n)]

   -p: specify the precision of PI.
   -w: specify the number of worker.
   -v: specify the verion of multithread implementation. Currently 1, 2, 3 is available, and default is 3.
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

## Some Related References
#### Algorithm to Calculate PI
- An response from the world record holder of the most digits of pi
    - https://stackoverflow.com/questions/14283270/how-do-i-determine-whether-my-calculation-of-pi-is-accurate
#### Verifier
- PI world record with sample data for verification
    - http://www.numberworld.org/digits/Pi/
#### y-cruncher
- PI calculator from scratch, and with benchmark to GMP
    - http://www.numberworld.org/ymp/v1.0/benchmarks.html

## Implementation Details
- 3 versions of multithread implementation
    - Version 1.
        - PQTMasterV1() distribute ReqPack into PQTWorkerV1().
        - After that, continuously receive RespPack from PQTWorkerV1(), then we can start to run CombinePQTMasterV1() "one by one".
        - CombinePQTMasterV1() distribute the ReqPack to PQTWorkerV1(), which do the MP multiplication in multithread worker.
        - After combined the RespPack, return it back to ComputePQTMasterV1() for further distribution.
        - It will have only 1 RespPack left in the end, and that is the result.
    - Version 2.
        - PQTMasterV2() distribute ReqPack into PQTWorkerV1().
        - After that, different from version 1, when sliding_window ready, send MP multiplication ReqPack to worker, and go ahead to the next sliding window without waiting.
        - Then, CombinePQTMasterV2() and CombinePQTMergerV2() will only do the MP addition based on worker result, and store the result back to ComputePQTMasterV2().
        - It will have only 1 RespPack left in the end, and that is the result.
    - Version 3.
        - Based on V2, V3 migrate the addition part into worker during CombinPQTMasterV3().
        - After optimize the memory allocation with shared_ptr, this performs the same as V2.
- 3 parts of multithread stage:
    - Part 1.
        - binary splitting into suitable number of batch (this number must be power of 2)
        - can use all cores
    - Part 2.
        - master distribute the sub-task(multiplication & addition) of merge to workers, then retrieve results from them
        - can use all cores
    - Part 3.
        - merge the final result
        - can use only 2 cores

