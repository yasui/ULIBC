# ULIBC


## Overview

ULIBC is a callable library for CPU and Memory affinity to applying NUMA-aware to a thread-parallel implementation. This library affects the scalability and performance on NUMA system, such as Intel Xeon, AMD Opteron, Oracle SPARC, IBM Power, and SGI UV series.

ULIBC provides some APIs to obtaining hardware topology and managing the position of each thread and data using "MPI rank"-like simple indices, starting at zero; the CPU socket index, physical-core index in each CPU socket, or thread index in each physical-core.

ULIBC is available at some operating systems (although we have only confirmed Linux, Solaris, and AIX). ULIBC detects available processing elements on current process, and can combine other libraries and tools; `KMP_AFFINITY`, `GOMP_CPU_AFFINITY`, the `numactl` tool, and some queueing systems.




## Descriptions

ULIBC detects the processor topology and the online processor list at runtime. Then, ULIBC constructs an affinity setting for online processor list considering the processor topology. ULIBC provides some APIs for managing and obtaining them.

#### Processor topology

Using ULIBC, each position of processors on the entire system is represented by a unique number and a unique tuple (Socket, Physical-Core, SMT). Each thread is assigned either processor by using assignment table between thread-index and processor-index. ULIBC provides some APIs for obtaining a placement (NUMA-node, NUMA-core) of each thread.

The following tables summerises
The third column represents an example for the possible numbers on a NUMA system with 4-socket, 8-core per socket, and 2-SMT per core.

Key             | Description         | e.g.) NUMA system (4-sockets x 8-cores x 2-SMT)
---             | ---                 | ---
Processor index | Processor           | 0, 1, 2, ..., 63
Socket index    | CPU socket          | 0, 1, 2, 3
Core   index    | Physical-core       | 0, 1, 2, ..., 7
SMT    index    | SMT                 | 0, 1

Key             | Description         | e.g.) NUMA system (4-sockets x 8-cores x 2-SMT)
---             | ---                 | ---
Thread index    | Thread              | 0, 1, 2, ..., 63
NUMA node index | NUMA node           | 0, 1, 2, 3
NUMA core index | NUMA local cores    | 0, 1, 2, ..., 15

#### CPU affinity

User can settle the affinity by `ULIBC_AFFINITY` environment as `ULIBC_AFFINITY`=_mapping_:_binding_.
ULIBC supports _mapping_ from two processor mappings { `compact`, `scatter`, `external` } and _binding_ from three binding levels { `fine`, `thread`, `core`, `socket` }.

* Two processor mappings
    + `compact` ... Specifying compact assigns threads in a position close to each other. However, it avoids assigning threads on a same physical core as possible as, when a system enables the hyper-threading.
    + `scatter` ... Specifying scatter distributes the threads as evenly as possible across the online (available) processors on the entire system.
    + `external` ... Specifying external do nothing for external affinity setting
* Three binding levels
    + `fine` (`thread`) ... Each thread binds into a logical processor.
    + `core` ... Each thread binds into online (available) logical processors on a same physical core.
    + `socket` ... Each thread binds into online (available) logical processors on a same socket.

#### Supported platform

ULIBC has three type implementation variants for different platforms; _Linux_ for Linux OS, _Hwloc_ for Unix OS, and _Dummy_ for other OS. The _Linux_ version uses `sched_{set,get}affinity` and `mbind` APIs, which are provides at Linux OS. The _Hwloc_ version uses HWLOC library APIs inside ULIBC, mainly uses on Unix system such as Solaris and AIX. The _Dummy_ version is for other OS that has no APIs for CPU and memory affinities. Usually, the user don't need to consider the several type, because ULIBC detects the running platform by `uname -s`. In this table, the _recommended_ indicates that we recommend the corresponding version at this platform.

Platform | binding APIs  | _Linux_     | _Hwloc_     | _Dummy_
---      | ---           | ---         | ---         | ---
Linux    | exist         | recommended | supported   | supported
Solaris  | exist         | --          | recommended | supported
AIX      | exist         | --          | recommended | supported
OSX      | not exist     | --          | supported   | recommended
Others   | not exist     | --          | supported   | recommended

#### Minimal code

Here is minimal code using ULIBC. At first, the `ULIBC_init()` function initializes the ULIBC data structure, in which ULIBC detects hardware topology runtime. Each thread is pinned on a processing element via `ULIBC_bind_thread_explicit()` at OpenMP region, and obtains a placement using `ULIBC_get_numainfo()`.

```
#include <stdio.h>
#include <omp.h>
#include <ulibc.h>

int main(void) {
  /* initialize ULIBC variables */
  ULIBC_init();

  /* OepnMP region with ULIBC affinity */
  _Pragma("omp parallel") {
    /* thread index */
    const int tid = ULIBC_get_thread_num();
    
    /* thread binding */
    ULIBC_bind_thread_explicit(tid);

    /* current NUMA placement */
    const struct numainfo_t loc = ULIBC_get_numainfo( tid );
    printf("%d thread is running on NUMA-node %d NUMA-core %d\n",
           loc.id, loc.node, loc.core);

    /* do something */
  }
}
```




## Installation

#### Requirements

+ C compiler (OpenMP supported)
    + GCC
    + Intel Compiler
    + Oracle Sun CC
    + IBM XLC
+ (Optional) [Hardware Locality (HWLOC)](https://www.open-mpi.org/projects/hwloc/)

We have tested on following systems.
+ Intel Xeon server
+ AMD Opteron server
+ SGI Altix UV 1000 (Linux version only)
+ SGI UV 2000 (Linux version only)
+ SGI UV 300 (Linux version only)
+ Oracle Sparc + Sun CC (Hwloc version only)
+ IBM + XLC (Hwloc version only)
+ OSX (HWLOC and dummy version only, but OSX has no binding APIs)

#### Installation

Just type the following command to install ULIBC.

```
$ make && make install
```

The default compiler is GCC. If you use another compiler, you can specify the C compiler using CC option as follows;

```
$ make CC=icc
```

The default target directory is `${HOME}/local`. If you change the directory, you can specify it using PREFIX option as follows;

```
$ make PREFIX=/usr/local
```

In order to avoiding some troubles, we recommend to use self-build HWLOC library.
When you use UNIX or setting _Hwloc_ version of ULIBC, Just type the following commands;

```
$ make HWLOC_BUILD=yes
```


In addition, if you want to use _Hwloc_ version at Linux and OSX, please add `OS=Hwloc` option.

```
$ make OS=Hwloc HWLOC_BUILD=yes
```




## Tutorials

#### Environment Variables

* `ULIBC_ALIGNSIZE=N`
    + Sets the alignment size in bytes to `N`. ULIBC uses this size in a first-touch after the memory allocation.
* `ULBIC_AVOID_HTCORE=BOOL`
    + 0: do nothing (default)
    + 1: Avoids assigning threads to same physical cores as possible as.
* `ULIBC_AFFINITY=MAPPING:BINDING`
    + Specifies the `MAPPING` to { `compact`, `scatter`, `external` } and the `BINDING` to { `fine`, `thread`, `core`, `socket` }.
* `ULIBC_USE_SCHED_AFFINITY=BOOL`  
    + 0: do nothing (default)
    + 1: Uses external affinity (ULIBC does not constructs an affinity setting)
* `ULIBC_PROCLIST=STRING`
    + Specify an available processor list using processor indices, '-', and ','.
    + c.g.) ULIBC_PROCLIST=0-3,8,19 indicates processors { 0, 1, 2, 3, 8, 19 }.
* `ULIBC_VERBOSE=N`
    + Set the verbose level to N.
    + 0: NOT prints some log (default)
    + 1: prints partial information
    + 2: prints all


#### Examples

###### Thread binding

`ULIBC_bind_thread()` binds a current thread into the corresponding processing element. It may not need to call this function, due to ULIBC constructs affinity for OpenMP threads in `ULIBC_init()`. This function is equivalent to `ULIBC_bind_thread_explicit( ULIBC_get_thread_num() );`.

```
ULIBC_init();
_Pragma("omp parallel") {
  ULIBC_bind_thread();
}
```

###### Thread binding explicitly

`ULIBC_bind_thread_explicit(tid)` binds a current thread into the corresponding processing element while setting a current thread index to a specified number _tid_.

```
_Pragma("omp parallel") {
  const int tid = omp_get_thread_num();
  ULIBC_bind_thread_explicit( tid );
}
```

###### NUMA-aware memory allocation

`NUMA_touched_malloc(size, k)` allocates a first-touched memory space with _size_ bytes on _k_-th the NUMA node (_k_ is not a socket number). `NUMA_free(p)` releases the memory space with address _p_.

```
const int64_t local_n = n / ell;
const int ell = ULIBC_get_online_nodes();

/* allocating NUMA-aware memory spaces */
double **vec_list = (double **)malloc(sizeof(double *) * ell);
for (int k = 0; k < ell; ++k) {
  lec_list[k] = NUMA_touched_malloc(local_n, k);
}

_Pragma("omp parallel") {
  const int tid = ULIBC_get_thread_num();
  const struct numainfo_t loc = ULIBC_get_numainfo( tid );

  /* NUMA-local variables */
  double *vec = lec_list[ loc.node ];

  /* do something */
}

for (int k = 0; k < ell; ++k) {
  NUMA_free(lec_list[k]);
}
```

###### NUNA-aware loops with dynamic load balancing

`ULIBC_numa_loop(chunksize,ls,le)` conducts a NUMA-aware dynamic load-balanced loop, in which each thread computes a partial range [_ls_,_le_) at each turn. The loop size (_le_-_ls_) is less than or equal to a chunk size _chunksize_. After initializing a ULIBC inside variable about loop range using `ULIBC_clear_numa_loop(begin, end)` for a range [_begin_,_end_), this function needs to synchronize it on NUMA local threads using `ULIBC_node_barrier()`.

```
const int64_t local_n = n / ell;
_Pragma("omp parallel") {
  const int tid = ULIBC_get_thread_num();
  const struct numainfo_t loc = ULIBC_get_numainfo( tid );

  /* NUMA-local variables */
  double *vec = lec_list[ loc.node ];
  int64_t ls, le;
  ULIBC_clear_numa_loop(0, local_n);

  /* NUMA local barrier sync. for ULIBC_clear_numa_loop() */
  ULIBC_node_barrier();

  while ( ULIBC_numa_loop(256, &ls, &le) ) { /* chunk size: 256 */
    for (int64_t i = ls; i < le; ++i)
      vec[i] = (double)i;
  }
}
```



## References

The basic idea of ULIBC is described in the following papers. If you use ULIBC, please REFERENCE this paper.

+ [BD13] Y\. Yasui, K. Fujisawa, and K. Goto; "NUMA-optimized parallel breadth-first search on multicore single-node system", 2013 IEEE International Conference on Big Data, 394-402, Oct 2013. [BD13](http://ieeexplore.ieee.org/xpl/articleDetails.jsp?arnumber=6691600&newsearch=true&queryText=NUMA-optimized%20parallel%20breadth-first%20search%20on%20multicore%20single-node%20system)

In addition to the above, we described ULIBC and its application Graph500 benchmark in the following papers.

+ [ISC14] Y\. Yasui, K. Fujisawa, and Y. Sato: "Fast and Energy-efficient Breadth-First Search on a Single NUMA System", Supercomputing Volume 8488 of the series Lecture Notes in Computer Science pp 365-381, 29th International Conference, ISC 2014, Leipzig, Germany, June 22-26, 2014. Proceedings. [ISC14](http://link.springer.com/chapter/10.1007%2F978-3-319-07518-1_23)

+ [HPCS15] Y\. Yasui and K. Fujisawa: "Fast and scalable NUMA-based thread parallel breadth-first search", The proceedings of the HPCS 2015 (The 2015 International Conference on High Performance Computing & Simulation), Amsterdam, the Netherlands, ACM, IEEE, IFIP, 2015. [HPCS15](http://ieeexplore.ieee.org/xpl/articleDetails.jsp?arnumber=7237065&newsearch=true&queryText=%20Fast%20and%20scalable%20NUMA-based%20thread%20parallel%20breadth-first%20search)

+ [HPGP16] Y\. Yasui and K. Fujisawa: "NUMA-aware scalable graph traversal on SGI UV systems", The proceedings of the HPGP'16 (The 1st High Performance Graph Processing workshop), HPDC 2016 conference, Kyoto, ACM, IEEE, 2016. [HPGP16](http://hpgp.bsc.es/accepted-papers)




## License

ULIBC is free software: you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

ULIBC is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License
along with ULIBC.  If not, see <http://www.gnu.org/licenses/>.


## Copyright

Copyright (C) 2013 - 2016 Yuichiro Yasui < yuichiro.yasui@gmail.com >