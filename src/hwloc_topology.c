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
#include <stdio.h>
#include <assert.h>
#include <hwloc.h>
#include <ulibc.h>
#include <common.h>

static char __version[256];
char *ULIBC_version(void) { return __version; }
void ULIBC_set_version(void) { 
  sprintf(__version, "ULIBC-%s-hwloc", ULIBC_VERSION);
}

static hwloc_topology_t __hwloc_topology;
hwloc_topology_t ULIBC_get_hwloc_topology(void) { return __hwloc_topology; }

static hwloc_obj_t __node_obj[MAX_NODES];
hwloc_obj_t ULIBC_get_node_hwloc_obj(int node) { return __node_obj[node]; }

static hwloc_obj_t __cpu_obj[MAX_CPUS];
hwloc_obj_t ULIBC_get_cpu_hwloc_obj(int cpu) { return __cpu_obj[cpu]; }

static size_t __pagesize[MAX_NODES] = {0};
static size_t __memorysize[MAX_NODES] = {0};
static size_t __alignsize = 0;
static int __cpuinfo_count = 0;
static int __num_procs;
static int __num_nodes;
static int __num_cores;
static int __num_smts;
static struct cpuinfo_t __cpuinfo[MAX_CPUS] = { {0,0,0,0} };

static bitmap_t hwloc_isonline_proc[MAX_CPUS/64] = {0};
static bitmap_t hwloc_isonline_node[MAX_NODES/64] = {0};

/* online_topology.c */
int __max_online_procs;
int __online_proclist[MAX_CPUS];
int __enable_online_procs;

/* for hwloc */
static int __online_nodes = 0;
static int __online_nodelist[MAX_NODES];
static int __online_ncores_on_node[MAX_NODES] = { 0 };

/* initialize_topology */
static void hwloc_topology_traversal(hwloc_topology_t topology, hwloc_obj_t obj, unsigned depth);

int ULIBC_init_topology(void) {
  double t;
  
  if ( ULIBC_verbose() )
    printf("ULIBC: HWLOC API version: %u\n", HWLOC_API_VERSION);
  
  /* get topology using HWLOC */
  PROFILED( t, hwloc_topology_init(&__hwloc_topology) );
  PROFILED( t, hwloc_topology_load(__hwloc_topology)  );
  __num_procs = hwloc_get_nbobjs_by_type(__hwloc_topology, HWLOC_OBJ_PU);
  __num_nodes = hwloc_get_nbobjs_by_type(__hwloc_topology, HWLOC_OBJ_NODE);
  __num_cores = hwloc_get_nbobjs_by_type(__hwloc_topology, HWLOC_OBJ_CORE);
  __num_smts  = __num_procs;
  
  if ( __num_procs > 0 )
    __enable_online_procs = 1;
  else 
    __enable_online_procs = 0;
  
  PROFILED( t, hwloc_topology_traversal(__hwloc_topology, hwloc_get_root_obj(__hwloc_topology), 0) );
  __num_nodes = __online_nodes;
  if ( __num_procs != __cpuinfo_count ) {
    printf("ULIBC: don't work hwloc_topology_traversal()\n");
    printf("ULIBC: # CPUs is %d, # CPUinfos is %d\n", __num_procs, __cpuinfo_count);
    exit(1);
  }
  __alignsize = getenvi("ULIBC_ALIGNSIZE", ULIBC_page_size(0));
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
  if ( ISSET_BITMAP(hwloc_isonline_proc, procidx) )
    return __cpuinfo[procidx];
  else
    return (struct cpuinfo_t){
      .id = -1, .node = -1, .core = -1, .smt = -1,
    };
}


/* CPU and Memory detection using HWLOC */
/* temporary variables for hwloc_topology_traversal() */
unsigned curr_node = 0;
unsigned curr_core = 0;
unsigned curr_smt[MAX_CPUS] = {0};

static void hwloc_topology_traversal(hwloc_topology_t topology, hwloc_obj_t obj, unsigned depth) {
  if (obj->type == HWLOC_OBJ_NODE) {
    /* if ( ULIBC_verbose() > 3 ) */
    /*   printf("ULIBC: Type: HWLOC_OBJ_NODE, os_index: %d, logical_index: %d\n", */
    /* 	     obj->os_index, obj->logical_index); */
    
    /* nodes */
    curr_node = obj->os_index;
    assert( !ISSET_BITMAP(hwloc_isonline_node, curr_node) );
    SET_BITMAP(hwloc_isonline_node, curr_node);
    memset(curr_smt, 0x00, sizeof(unsigned)*MAX_CPUS);
    curr_core = -1;
    __memorysize[curr_node] = obj->memory.local_memory;
    for (unsigned i = 0; i < obj->memory.page_types_len; ++i) {
      __pagesize[curr_node] = obj->memory.page_types[i].size;
    }
    __node_obj[curr_node] = obj;

    /* online nodes */
    __online_nodelist[__online_nodes++] = curr_node;
  }
  
  if (obj->type == HWLOC_OBJ_CORE) {
    ++curr_core;
  }
  
  if (obj->type == HWLOC_OBJ_PU) {
    /* if ( ULIBC_verbose() > 3 ) */
    /*   printf("ULIBC: Type: HWLOC_OBJ_PU, os_index: %d, logical_index: %d\n", */
    /* 	     obj->os_index, obj->logical_index); */
    const unsigned proc = obj->os_index;
    assert( !ISSET_BITMAP(hwloc_isonline_proc, proc) );
    SET_BITMAP(hwloc_isonline_proc, proc);
    
    /* cpus */
    __cpuinfo_count++;
    __cpuinfo[proc] = (struct cpuinfo_t){
      .id = proc,
      .node = curr_node,
      .core = curr_core,
      .smt = curr_smt[curr_core]++,
    };
    __cpu_obj[proc] = obj;	/* hwloc_obj_t */
    
    /* online cpus */
    __online_proclist[__max_online_procs++] = proc;
    __online_ncores_on_node[curr_node]++;
  }
  for (unsigned i = 0; i < obj->arity; ++i) {
    hwloc_topology_traversal(topology, obj->children[i], depth + 1);
  }
}

/* detection function */
int is_online_proc(int proc) {
  hwloc_cpuset_t cpuset = hwloc_bitmap_alloc();
  hwloc_bitmap_zero(cpuset);
  hwloc_get_cpubind(ULIBC_get_hwloc_topology(), cpuset, HWLOC_CPUBIND_THREAD);
  const int ret = hwloc_bitmap_isset(cpuset, proc);
  hwloc_bitmap_free(cpuset);
  return ret;
}
