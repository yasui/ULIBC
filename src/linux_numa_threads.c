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
#include <assert.h>
#include <sched.h>

#include <ulibc.h>
#include <common.h>

static __thread int initialized = 0;
static __thread cpu_set_t __bind_cpuset;
static cpu_set_t __default_cpuset;

int ULIBC_init_numa_threads(void) {
  if ( ULIBC_use_affinity() == NULL_AFFINITY ) return 1;
  
  /* get default (master-thread) affinity */
  sched_getaffinity( (pid_t)0, sizeof(cpu_set_t), &__default_cpuset );
  
  /* set openmp-thread affinity */
  OMP("omp parallel") {
    ULIBC_bind_thread();
  }
  
  /* print */
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
  cpu_set_t cpuset;
  CPU_ZERO(&cpuset);
  CPU_SET(ni.proc, &cpuset);
  
  switch ( ULIBC_get_current_binding() ) {
  case THREAD_TO_THREAD: break;
    
  case THREAD_TO_CORE: {
    struct cpuinfo_t ci = ULIBC_get_cpuinfo( ni.proc );
    for (int u = 0; u < ULIBC_get_online_procs(); ++u) {
      struct cpuinfo_t cj = ULIBC_get_cpuinfo( ULIBC_get_numainfo(u).proc );
      if ( ci.node == cj.node && ci.core == cj.core )
  	CPU_SET(cj.id, &cpuset);
    }
    break;
  }
  case THREAD_TO_SOCKET: {
    struct cpuinfo_t ci = ULIBC_get_cpuinfo( ni.proc );
    for (int u = 0; u < ULIBC_get_online_procs(); ++u) {
      struct cpuinfo_t cj = ULIBC_get_cpuinfo( ULIBC_get_numainfo(u).proc );
      if ( ci.node == cj.node )
  	CPU_SET(cj.id, &cpuset);
    }
    break;
  }
  default: break;
  }
  
  /* binds */
  sched_setaffinity( (pid_t)0, sizeof(cpu_set_t), &cpuset );
  sched_getaffinity( (pid_t)0, sizeof(cpu_set_t), &__bind_cpuset );
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
    CPU_ZERO(&__bind_cpuset);
    initialized = 1;
  }
  
  cpu_set_t set;
  assert( !sched_getaffinity((pid_t)0, sizeof(cpu_set_t), &set) );
  
  if ( ! CPU_EQUAL(&set, &__bind_cpuset) ) {
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
  sched_setaffinity((pid_t)0, sizeof(cpu_set_t), &__default_cpuset);
  return 1;
}

int ULIBC_is_bind_thread(int proc) {
  return CPU_ISSET(proc, &__bind_cpuset);
}
