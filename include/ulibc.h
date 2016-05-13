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
#ifndef ULIBC_H
#define ULIBC_H

#ifndef ULIBC_VERSION
#define ULIBC_VERSION "1.20"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

/* ------------------------------------------------------------------------------- *
 *
 * ULIBC_ALIGNSIZE (default: auto)
 *   alignment size in bytes
 *   Usage: ULIBC_ALIGNSIZE=4096 ./a.out
 *
 * ULBIC_AVOID_HTCORE (default: 0)
 *   set low priority to Hyperthteading (HT) cores for avoiding
 *   Usage: ULBIC_AVOID_HTCORE=1 ./a.out
 *
 * ULIBC_AFFINITY (default: scatter:core)
 *   set affinity-types {scatter, compact} and affinity-bind-levels {socket, core, thread, fine}
 *   Usage: ULIBC_AFFINITY=compact:fine ./a.out
 *
 * ULIBC_USE_SCHED_AFFINITY (default: 0)
 *   uses scheduler affinity
 *   Usage: ULIBC_USE_SCHED_AFFINITY=1 KMP_AFFINITY=compact,granularity=fine ./a.out
 *
 * ULIBC_PROCLIST (default: not used)
 *   processor list
 *   Usage: ULIBC_PROCLIST=0-3 ./a.out
 *
 * ULIBC_VERBOSE (default: 0)
 *   verbose level = {0,1,2}
 *   Usage: ULIBC_VERBOSE=1 ./a.out
 *
 * ------------------------------------------------------------------------------- */

#if defined (__cplusplus)
extern "C" {
#endif
  /* init.c */
  int ULIBC_init(void);
  int ULIBC_verbose(void);
  char *ULIBC_version(void);  
  
  /* topology.c */
  int ULIBC_get_num_procs(void);
  int ULIBC_get_num_nodes(void);
  int ULIBC_get_num_cores(void);
  int ULIBC_get_num_smts(void);
  size_t ULIBC_page_size(unsigned nodeidx);
  size_t ULIBC_memory_size(unsigned nodeidx);
  size_t ULIBC_total_memory_size(void);
  size_t ULIBC_align_size(void);
  struct cpuinfo_t {
    int id;			/* Processor ID */
    int node;			/* Package ID */
    int core;			/* Core ID */
    int smt;			/* SMT ID */
  };
  struct cpuinfo_t ULIBC_get_cpuinfo(unsigned procidx);
  
  /* print functions */
  void ULIBC_print_topology(FILE *fp);
  void ULIBC_print_online_topology(FILE *fp);
  void ULIBC_print_mapping(FILE *fp);
  void ULIBC_print_thread_mapping(FILE *fp);
  void ULIBC_print_main_thread_binding(FILE *fp);
  void ULIBC_print_openmp_binding(FILE *fp);
  
  /* online_topology.c */
  int ULIBC_get_max_online_procs(void);
  int ULIBC_enable_online_procs(void);
  int ULIBC_get_online_procidx(unsigned idx);
  
  /* numa_mapping.c */
  void ULIBC_clear_thread_num(void);
  int ULIBC_get_thread_num(void);
  int ULIBC_use_affinity(void);
  int ULIBC_enable_numa_mapping(void);
  int ULIBC_get_current_mapping(void);
  int ULIBC_get_current_binding(void);
  const char *ULIBC_get_current_mapping_name(void);
  const char *ULIBC_get_current_binding_name(void);
  
  int ULIBC_get_online_procs(void);
  int ULIBC_get_online_nodes(void);
  int ULIBC_get_online_cores(int node);
  int ULIBC_get_online_nodeidx(int node);

  int ULIBC_get_num_threads(void);
  void ULIBC_set_num_threads(int nt);
  enum affinity_type_t {
    NULL_AFFINITY,
    ULIBC_AFFINITY,
    SCHED_AFFINITY
  };
  enum map_policy_t {
    SCATTER_MAPPING = 0x00,
    COMPACT_MAPPING = 0x01,
  };
  enum bind_level_t {
    THREAD_TO_THREAD = 0x00,
    THREAD_TO_CORE   = 0x01,
    THREAD_TO_SOCKET = 0x02,
  };
  int ULIBC_set_affinity_policy(int nt, int map, int bind);
  struct numainfo_t {
    int id;		      /* Thread ID */
    int proc;		      /* Processor ID for "cpuinfo_t" */
    int node;		      /* NUMA node ID */
    int core;		      /* NUMA core ID */
    int lnp;		      /* Number of NUMA cores in NUMA node */
  };
  struct numainfo_t ULIBC_get_numainfo(int tid);
  struct numainfo_t ULIBC_get_current_numainfo(void);
  
  /* threading */
  int ULIBC_bind_thread(void);
  int ULIBC_bind_thread_explicit(int threadid);
  int ULIBC_unbind_thread(void);
  int ULIBC_is_bind_thread(int proc);
  void ULIBC_clear_numa_loop(int64_t loopstart, int64_t loopend);
  int ULIBC_numa_loop(int64_t chunk, int64_t *start, int64_t *end);
  
  /* barrier */
  void ULIBC_barrier(void);
  void ULIBC_node_barrier(void);  
  void ULIBC_pair_barrier(int node_s, int node_t);
  void ULIBC_hierarchical_barrier(void);
  
  /* malloc */
  char *ULIBC_get_memory_name(void);
  void *NUMA_malloc(size_t size, const int onnode);
  void *NUMA_touched_malloc(size_t sz, int onnode);
  void ULIBC_touch_memories(size_t size[], void *pool[]);
  void NUMA_free(void *p);
  
  /* tools.c */
  double get_msecs(void);
  unsigned long long get_usecs(void);
  long long getenvi(char *env, long long def);
  double getenvf(char *env, double def);
  size_t uniq(void *base, size_t nmemb, size_t size,
	      void (*sort)(void *base, size_t nmemb, size_t size,
			   int(*compar)(const void *, const void *)),
	      int (*compar)(const void *, const void *));
  void uheapsort(void *base, size_t nmemb, size_t size,
		 int (*compar)(const void *, const void *));
#if defined (__cplusplus)
}
#endif
#endif /* ULIBC_H */
