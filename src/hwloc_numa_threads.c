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
#include <stdlib.h>
#include <math.h>
#include <assert.h>
#include <hwloc.h>

#include <ulibc.h>
#include <common.h>

static __thread int initialized;
static __thread hwloc_cpuset_t __bind_cpuset;
static __thread hwloc_cpuset_t __curr_cpuset;
static __thread hwloc_topology_t __hwloc_topology_local;
static hwloc_cpuset_t __default_cpuset;

extern hwloc_topology_t ULIBC_get_hwloc_topology(void);
extern hwloc_obj_t ULIBC_get_cpu_hwloc_obj(int cpu);

int ULIBC_init_numa_threads(void) {
  if ( ULIBC_use_affinity() == NULL_AFFINITY ) return 1;

  /* get default (master-thread) affinity */
  __default_cpuset = hwloc_bitmap_alloc();
  hwloc_bitmap_zero( __default_cpuset );
  hwloc_get_cpubind( ULIBC_get_hwloc_topology(), __default_cpuset, HWLOC_CPUBIND_THREAD );

  OMP("omp parallel") {
    ULIBC_bind_thread();
  }

  if ( ULIBC_verbose() ) {
    ULIBC_print_thread_mapping(stdout);
    ULIBC_print_main_thread_binding(stdout);
    ULIBC_print_openmp_binding(stdout);
  }
  
  return 0;
}

static void bind_thread(const int id) {
  if ( ULIBC_use_affinity() != ULIBC_AFFINITY ) return;
  
  struct numainfo_t ni = ULIBC_get_numainfo(id);
  
  /* constructs cpuset */
  hwloc_cpuset_t cpuset = hwloc_bitmap_alloc();
  hwloc_bitmap_zero(cpuset);
  hwloc_bitmap_or( cpuset, cpuset, ULIBC_get_cpu_hwloc_obj(ni.proc)->cpuset );
  
  switch ( ULIBC_get_current_binding() ) {
  case THREAD_TO_THREAD: break;
    
  case THREAD_TO_CORE: {
    struct cpuinfo_t ci = ULIBC_get_cpuinfo( ni.proc );
    for (int u = 0; u < ULIBC_get_online_procs(); ++u) {
      struct cpuinfo_t cj = ULIBC_get_cpuinfo( ULIBC_get_numainfo(u).proc );
      if ( ci.node == cj.node && ci.core == cj.core )
	hwloc_bitmap_or( cpuset, cpuset, ULIBC_get_cpu_hwloc_obj(cj.id)->cpuset );
    }
    break;
  }
  case THREAD_TO_SOCKET: {
    struct cpuinfo_t ci = ULIBC_get_cpuinfo( ni.proc );
    for (int u = 0; u < ULIBC_get_online_procs(); ++u) {
      struct cpuinfo_t cj = ULIBC_get_cpuinfo( ULIBC_get_numainfo(u).proc );
      if ( ci.node == cj.node )
	hwloc_bitmap_or( cpuset, cpuset, ULIBC_get_cpu_hwloc_obj(cj.id)->cpuset );
    }
    break;
  }
  default: break;
  }
  
  /* binds */
  hwloc_set_cpubind( __hwloc_topology_local, cpuset, HWLOC_CPUBIND_THREAD );
  hwloc_bitmap_copy( __bind_cpuset, cpuset );
  hwloc_bitmap_free(cpuset);
}

int ULIBC_bind_thread_explicit(int threadid) {
  if ( ULIBC_use_affinity() != ULIBC_AFFINITY ) return 0;
  
  if ( ULIBC_verbose() > 1 ) {
    struct numainfo_t loc = ULIBC_get_numainfo( threadid );
    printf("ULIBC: binding thread (Rank %d) to Proc %d ( NUMA-Node: %d, NUMA-Core: %d )\n",
	   loc.id, loc.proc, loc.node, loc.core);
  }
  bind_thread( threadid );
  return 1;
}

int ULIBC_bind_thread(void) {
  if ( ULIBC_use_affinity() != ULIBC_AFFINITY ) return 0;
  
  if ( !initialized ) {
    __bind_cpuset = hwloc_bitmap_alloc();
    __curr_cpuset = hwloc_bitmap_alloc();
    hwloc_bitmap_zero( __bind_cpuset );
    hwloc_bitmap_zero( __curr_cpuset );
    hwloc_topology_dup( &__hwloc_topology_local, ULIBC_get_hwloc_topology() );
    initialized = 1;
  }
  
  hwloc_get_cpubind( __hwloc_topology_local, __curr_cpuset, HWLOC_CPUBIND_THREAD );
  
  if ( ! hwloc_bitmap_isequal(__bind_cpuset, __curr_cpuset) ) {
    if ( ULIBC_verbose() > 1 ) {
      struct numainfo_t loc = ULIBC_get_numainfo( ULIBC_get_thread_num() );
      printf("ULIBC: binding thread (Rank %d) to Proc %d ( NUMA-Node: %d, NUMA-Core: %d )\n",
	     loc.id, loc.proc, loc.node, loc.core);
    }
    bind_thread( ULIBC_get_thread_num() );
    return 1;
  }
  
  return 0;
}

int ULIBC_unbind_thread(void) {
  if ( ULIBC_use_affinity() != ULIBC_AFFINITY ) return 0;
  hwloc_set_cpubind( __hwloc_topology_local, __default_cpuset, HWLOC_CPUBIND_THREAD );
  return 1;
}

int ULIBC_is_bind_thread(int proc) {
  return hwloc_bitmap_isset(__bind_cpuset, proc);
}
