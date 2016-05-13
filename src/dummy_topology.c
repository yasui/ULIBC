/* ---------------------------------------------------------------------- *
 *
 * Copyright (C) 2013-2016 Yuichiro Yasui < yuichiro.yasui@gmail.com >
 *
 * This file is part of ULIBC.
 *
 * ULIBC is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * ULIBC is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with ULIBC.  If not, see <http://www.gnu.org/licenses/>.
 * ---------------------------------------------------------------------- */
#include <ulibc.h>
#include <common.h>

static char __version[256];
char *ULIBC_version(void) { return __version; }
void ULIBC_set_version(void) { 
  sprintf(__version, "ULIBC-%s-dummy", ULIBC_VERSION);
}

static size_t __pagesize[MAX_NODES] = {0};
static size_t __memorysize[MAX_NODES] = {0};
static size_t __alignsize = 0;
static int __cpuinfo_count = 0;
static int __num_procs;
static int __num_nodes;
static int __num_cores;
static int __num_smts;
static struct cpuinfo_t __cpuinfo[MAX_CPUS] = { {0,0,0,0} };

/* initialize_topology */
static void dummy_topology_traversal(void);

int ULIBC_init_topology(void) {
  double t;
  
  /* get topology using HWLOC */
  if ( ULIBC_verbose() )
    printf("HWLOC: API version: [unused]\n");
  
  __num_procs = omp_get_num_procs();
  __num_cores = omp_get_num_procs();
  __num_nodes = 1;
  __num_smts  = omp_get_num_procs();
  PROFILED( t, dummy_topology_traversal() );
  
  if ( __num_procs != __cpuinfo_count ) {
    printf("ULIBC: cannot cpuinfos\n");
    printf("ULIBC: # CPUs is %d, # CPUinfos is %d\n", __num_procs, __cpuinfo_count);
    exit(1);
  }
  __alignsize = getenvi("ULIBC_ALIGNSIZE", ULIBC_page_size(0));

  /* Check minimum node index  */
  int min_node = MAX_NODES;
  for (int i = 0; i < __cpuinfo_count; ++i) {
    min_node = MIN(min_node, __cpuinfo[i].node);
  }
  if ( min_node < 0 ) {
    printf("ULIBC: min. node index is %d\n", min_node);
    exit(1);
  }
  
  if (__num_nodes == 0) __num_nodes = 1;
  if (__alignsize == 0) __alignsize = 4096;
  
  if ( ULIBC_verbose() ) {
    printf("ULIBC: # of Processor cores is %4d\n", ULIBC_get_num_procs());
    printf("ULIBC: # of NUMA nodes      is %4d\n", ULIBC_get_num_nodes());
    printf("ULIBC: # of Cores           is %4d\n", ULIBC_get_num_cores());
    printf("ULIBC: # of SMTs            is %4d\n", ULIBC_get_num_smts());
    printf("ULIBC: Alignment size       is %ld bytes\n", ULIBC_align_size());
  }
  
  if ( ULIBC_verbose() > 1 ) {
    ULIBC_print_topology(stdout);
  }
  return 0;
}



/* get number of processors, packages, cores/socket, and threads/core, page size, memory size */
int ULIBC_get_num_procs(void) { return __num_procs; }
int ULIBC_get_num_nodes(void) { return __num_nodes; }
int ULIBC_get_num_cores(void) { return __num_cores; }
int ULIBC_get_num_smts(void) { return __num_smts; }
size_t ULIBC_page_size(unsigned nodeidx) { return __pagesize[nodeidx]; }
size_t ULIBC_memory_size(unsigned nodeidx) { return __memorysize[nodeidx]; }
size_t ULIBC_align_size(void) { return __alignsize; }

size_t ULIBC_total_memory_size(void) {
  static size_t total = 0;
  if (total == 0) {
    for (int i = 0; i < ULIBC_get_num_nodes(); ++i)
      total += ULIBC_memory_size(i);
  }
  return total;
}

struct cpuinfo_t ULIBC_get_cpuinfo(unsigned procidx) {
  if (ULIBC_get_num_procs() <= (int)procidx)
    procidx %= ULIBC_get_num_procs();
  return __cpuinfo[procidx];
}

/* dummy function for detecting CPU and Memory topology */
static void dummy_topology_traversal(void) {
  __memorysize[0] = 0;
  __pagesize[0] = 4096;
  __cpuinfo_count = ULIBC_get_num_procs();
  for (int i = 0; i < ULIBC_get_num_procs(); ++i) {
    __cpuinfo[i] = (struct cpuinfo_t){
      .id = i,
      .node = 0,
      .core = i,
      .smt = 0,
    };
  }
}


/* detection functions for online processor */
int is_online_proc(int proc) {
  (void)proc;
  return 1;
}
