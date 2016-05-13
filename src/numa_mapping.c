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
#include <string.h>

#include <ulibc.h>
#include <common.h>

/* affinity policy */
static int __avoid_htcore = 0;
static int __use_affinity = NULL_AFFINITY;
static int __mapping_policy = SCATTER_MAPPING;
static int __binding_policy = THREAD_TO_THREAD;

/* affinity */
static int __online_procs;
static int __online_nodes;
static int __online_ncores_on_node[MAX_NODES];
static int __online_nodelist[MAX_NODES];
struct numainfo_t __numainfo[MAX_CPUS];

static void get_sorted_procs(int *sorted_proc);
static int make_numainfo(int *sorted_proc);

/* ------------------------------------------------------------
 * Init. function
 * ------------------------------------------------------------ */
int ULIBC_init_numa_mapping(void) {
  __avoid_htcore = getenvi("ULBIC_AVOID_HTCORE", 0);
  
  /* policy */
  const char *affinity_env = getenv("ULIBC_AFFINITY");
  if ( affinity_env ) {
    char buff[256];
    strcpy(buff, affinity_env);
    char *affi_name = strtok(buff, ":");
    char *bind_name = strtok(NULL, ":");
    if ( affi_name ) {
      if      ( !strcmp(affi_name, "external") ) __use_affinity   = SCHED_AFFINITY;
      else if ( !strcmp(affi_name, "scatter")  ) { __use_affinity = ULIBC_AFFINITY; __mapping_policy = SCATTER_MAPPING; }
      else if ( !strcmp(affi_name, "compact")  ) { __use_affinity = ULIBC_AFFINITY; __mapping_policy = COMPACT_MAPPING; }
      else {
	printf("Unkrown affinity policy '%s'.\n"
	       "    ULIBC supports 'scatter' or 'compact'.\n", affi_name);
	exit(1);
      }
    }
    if ( bind_name ) {
      if      ( !strcmp(bind_name, "thread") ) __binding_policy = THREAD_TO_THREAD;
      else if ( !strcmp(bind_name, "fine")   ) __binding_policy = THREAD_TO_THREAD;
      else if ( !strcmp(bind_name, "core")   ) __binding_policy = THREAD_TO_CORE;
      else if ( !strcmp(bind_name, "socket") ) __binding_policy = THREAD_TO_SOCKET;
      else {
	printf("Unkrown binding policy '%s'.\n"
	       "    ULIBC supports 'thread' ('fine'), 'core', or 'socket'.\n", bind_name);
	exit(1);
      }
    }
  }
  
  /* affinity settings */
  extern int __detect_external_affinity;
  if ( !ULIBC_enable_online_procs() ) {
    __use_affinity = NULL_AFFINITY;
  } else {
    if ( /* explicitly specified */
	__use_affinity == SCHED_AFFINITY ||
	
	/* implicitly specified */
	( __use_affinity != ULIBC_AFFINITY &&
	  __detect_external_affinity &&
	  ( getenvi("ULIBC_USE_SCHED_fAFFINITY",0) || 
	    getenv("KMP_AFFINITY") || 
	    getenv("GOMP_CPU_AFFINITY") ) ) ) {
      __use_affinity = SCHED_AFFINITY;
    } else {
      __use_affinity = ULIBC_AFFINITY;
    }
  }
  if (ULIBC_verbose())
    printf("ULIBC: ULIBC_enable_online_procs(): %d\n", ULIBC_enable_online_procs());
  if ( ULIBC_verbose() ) {
    printf("ULIBC: ULBIC_AVOID_HTCORE=%d\n", __avoid_htcore);
    printf("ULIBC: ULIBC_AFFINITY=%s:%s\n",
	   ULIBC_get_current_mapping_name(), ULIBC_get_current_binding_name());
    if (getenv("KMP_AFFINITY"))
      printf("ULIBC: KMP_AFFINITY='%s'\n", getenv("KMP_AFFINITY"));
    else
      printf("ULIBC: KMP_AFFINITY=NULL\n");
    if (getenv("GOMP_CPU_AFFINITY"))
      printf("ULIBC: GOMP_CPU_AFFINITY='%s'\n", getenv("GOMP_CPU_AFFINITY"));
    else
      printf("ULIBC: GOMP_CPU_AFFINITY=NULL\n");
  }
  
  /* fill the processor list */
  int proc_list[MAX_CPUS];
  TIMED( get_sorted_procs(proc_list) );
  
  if ( ULIBC_verbose() ) {
    for (int i = 0; i < ULIBC_get_max_online_procs(); ++i) {
      const int idx = proc_list[i];
      struct cpuinfo_t ci = ULIBC_get_cpuinfo(idx);
      printf("ULIBC: Online CPU[%03d] Processor: %3d, Package: %2d, Core: %2d, SMT: %2d\n",
	     idx, ci.id, ci.node, ci.core, ci.smt);
    }
  }
  
  /* threads */
  __online_procs = getenvi("OMP_NUM_THREADS", ULIBC_get_max_online_procs());
  __online_procs = MIN(__online_procs, ULIBC_get_max_online_procs());
  omp_set_num_threads(__online_procs);
  if ( ULIBC_verbose() ) {
    printf("ULIBC: #online cpus is %d\n", __online_procs);
    printf("ULIBC: omp_set_num_threads=%d\n", __online_procs);
  }
  
  /* NUMA layout infos */
  TIMED( __online_nodes = make_numainfo(proc_list) );
  
  if ( ULIBC_verbose() )
    ULIBC_print_mapping(stdout);
  
  return 0;
}


/* ------------------------------------------------------------
 * Set/Get functions
 * ------------------------------------------------------------ */
int ULIBC_use_affinity(void) { return __use_affinity; }
int ULIBC_enable_numa_mapping(void) {
  switch ( ULIBC_use_affinity() ) {
  case ULIBC_AFFINITY:
  case SCHED_AFFINITY:
    return 1;
  case NULL_AFFINITY:
  default:
    return 0;
  }
}

int ULIBC_get_current_mapping(void) { return __mapping_policy; }
int ULIBC_get_current_binding(void) { return __binding_policy; }
const char *ULIBC_get_current_mapping_name(void) {
  if ( ULIBC_enable_numa_mapping() ) {
    switch ( ULIBC_use_affinity() ) {
    case ULIBC_AFFINITY:
      switch ( ULIBC_get_current_mapping() ) {
      case SCATTER_MAPPING: return "scatter";
      case COMPACT_MAPPING: return "compact";
      default:              return "unknown";
      }
      
    case SCHED_AFFINITY:
      return "external";
      
    case NULL_AFFINITY:
    default:
      return "unknown";
    }
  } else {
    return "disable";
  }
}
const char *ULIBC_get_current_binding_name(void) {
  if ( ULIBC_enable_numa_mapping() ) {
    switch ( ULIBC_use_affinity() ) {
    case ULIBC_AFFINITY:
      switch ( ULIBC_get_current_binding() ) {
      case THREAD_TO_THREAD: return "thread";
      case THREAD_TO_CORE:   return "core";
      case THREAD_TO_SOCKET: return "socket";
      default:               return "unknown";
      }
      
    case SCHED_AFFINITY:
      return "fine";
      
    case NULL_AFFINITY:
    default:
      return "unknown";
    }
  } else {
    return "disable";
  }
}

int ULIBC_get_online_procs(void) { return __online_procs; }
int ULIBC_get_online_nodes(void) { return __online_nodes; }
int ULIBC_get_online_cores(int node) { return __online_ncores_on_node[node]; }
int ULIBC_get_online_nodeidx(int node) {
  if ( node > ULIBC_get_online_nodes() )
    node %= ULIBC_get_online_nodes();
  return __online_nodelist[node];
}

int ULIBC_get_num_threads(void) {
  return __online_procs;
}
void ULIBC_set_num_threads(int nt) {
  omp_set_num_threads(nt);
  ULIBC_set_affinity_policy(nt, ULIBC_get_current_mapping(), ULIBC_get_current_binding());
}
int ULIBC_set_affinity_policy(int nt, int map, int bind) {
  /* set */
  omp_set_num_threads(nt);
  __mapping_policy = map;
  __binding_policy = bind;
  
  /* init. */
  ULIBC_init_numa_mapping();
  ULIBC_init_numa_threads();
  ULIBC_init_numa_barriers();
  ULIBC_init_numa_loops();
  return 0;
}


/* get numa info */
struct numainfo_t ULIBC_get_numainfo(int tid) {
  if (ULIBC_get_max_online_procs() <= tid) {
    tid %= ULIBC_get_max_online_procs();
  }
  if ( !ULIBC_enable_numa_mapping() || tid < 0 ) {
    return (struct numainfo_t){
      .id = tid, .proc = tid, .node = 0, .core = tid,
	.lnp = ULIBC_get_max_online_procs() };
  } else {
    return __numainfo[tid];
  }
}

/* get numa info of current thread */
struct numainfo_t ULIBC_get_current_numainfo(void) {
  struct numainfo_t ni = ULIBC_get_numainfo( ULIBC_get_thread_num() );
  ULIBC_bind_thread();
  return ni;
}


/* ------------------------------------------------------------
 * print numa mapping
 * ------------------------------------------------------------ */
void ULIBC_print_mapping(FILE *fp) {
  if (!fp) return;
  
  fprintf(fp, "ULIBC: # of NUMA nodes is %d\n", ULIBC_get_online_nodes());
  
  for (int j = 0; j < ULIBC_get_online_nodes(); ++j) {
    const int node = ULIBC_get_online_nodeidx(j);
    fprintf(fp, "ULIBC: NUMA %03d on Package %03d has %d NUMA-threads = { ",
	    j, node, ULIBC_get_online_cores(j));
    for (int i = 0; i < ULIBC_get_online_procs(); ++i) {
      struct numainfo_t ni = ULIBC_get_numainfo(i);
      if ( ULIBC_get_cpuinfo(ni.proc).node == node ) {
    	fprintf(fp, "%d ", i);
      }
    }
    fprintf(fp, "}\n");
  }
  for (int k = 0; k < ULIBC_get_online_nodes(); ++k) {
    const int node = ULIBC_get_online_nodeidx(k);
    int ncores = 0;
    char bind_str[MAX_CPUS+1];
    for (int i = 0; i < ULIBC_get_num_procs(); ++i) bind_str[i] = '-';
    bind_str[ ULIBC_get_num_procs() ] = '\0';
    for (int i = 0; i < ULIBC_get_num_procs(); ++i) {
      struct numainfo_t ni = ULIBC_get_numainfo(i);
      if ( ULIBC_get_cpuinfo( ni.proc ).node == node ) {
  	bind_str[ ni.proc ] = 'x';
  	ncores++;
      }
    }
    fprintf(fp, "ULIBC: NUMA-node %3d has %2d NUMA-cores    { %s }\n",
  	    k, ULIBC_get_online_cores(k), bind_str);
  }
}


void ULIBC_print_thread_mapping(FILE *fp) {
  if (!fp) return;
  for (int i = 0; i < ULIBC_get_online_procs(); ++i) {
    struct numainfo_t ni = ULIBC_get_numainfo(i);
    fprintf(fp, "ULIBC: OpenMP Thread %3d, NUMA: %3d, LCore: %2d, #LCores, %2d\n",
	    ni.id, ni.node, ni.core, ni.lnp);
  }
}


/* ------------------------------------------------------------
 * get processor list for CPU affinity
 * ------------------------------------------------------------ */
static int cmpr_scatter(const void *a, const void *b);
static int cmpr_compact(const void *a, const void *b);
static int cmpr_compact_avoid_ht(const void *a, const void *b);

static void get_sorted_procs(int *proc_list) {
  /* fill sorted_list with online processors */
  for (int i = 0; i < ULIBC_get_max_online_procs(); ++i) {
    proc_list[i] = ULIBC_get_online_procidx(i);
  }
  if ( ULIBC_verbose() > 1 ) {
    printf("ULIBC: Before: ");
    for (int i = 0; i < ULIBC_get_max_online_procs(); ++i) {
      printf("%d ", proc_list[i]);
    }
    printf("\n");
  }
  
  /* detecting or sorting */
  if ( __use_affinity == ULIBC_AFFINITY ) {
    switch ( __mapping_policy ) {
    case SCATTER_MAPPING:
      qsort(proc_list, ULIBC_get_max_online_procs(), sizeof(int), cmpr_scatter);
      break;
    case COMPACT_MAPPING:
      qsort(proc_list, ULIBC_get_max_online_procs(), sizeof(int),
	    (__avoid_htcore) ? cmpr_compact_avoid_ht : cmpr_compact);
      break;
    }
  }
  if ( ULIBC_verbose() > 1 ) {
    printf("ULIBC: After: ");
    for (int i = 0; i < ULIBC_get_max_online_procs(); ++i)
      printf("%d ", proc_list[i]);
    printf("\n");
  }
}

static int cmpr_proc(const void *a, const void *b);
static int cmpr_core(const void *a, const void *b);
static int cmpr_node(const void *a, const void *b);
static int cmpr_smt (const void *a, const void *b);

static int cmpr_scatter(const void *a, const void *b) {
  int ret;
  if ( (ret = cmpr_smt (a,b)) != 0 ) return ret;
  if ( (ret = cmpr_core(a,b)) != 0 ) return ret;
  if ( (ret = cmpr_node(a,b)) != 0 ) return ret;
  if ( (ret = cmpr_proc(a,b)) != 0 ) return ret;
  return ret;
}

static int cmpr_compact_avoid_ht(const void *a, const void *b) {
  int ret = 0;
  if ( (ret = cmpr_smt (a,b)) != 0 ) return ret;
  if ( (ret = cmpr_node(a,b)) != 0 ) return ret;
  if ( (ret = cmpr_core(a,b)) != 0 ) return ret;
  if ( (ret = cmpr_proc(a,b)) != 0 ) return ret;
  return ret;
}
static int cmpr_compact(const void *a, const void *b) {
  int ret = 0;
  if ( (ret = cmpr_node(a,b)) != 0 ) return ret;
  if ( (ret = cmpr_smt (a,b)) != 0 ) return ret;
  if ( (ret = cmpr_core(a,b)) != 0 ) return ret;
  if ( (ret = cmpr_proc(a,b)) != 0 ) return ret;
  return ret;
}

static int cmpr_proc(const void *a, const void *b) {
  const int _proc_a = ULIBC_get_cpuinfo(*(int *)a).id;
  const int _proc_b = ULIBC_get_cpuinfo(*(int *)b).id;
  if ( _proc_a < _proc_b) return -1;
  if ( _proc_a > _proc_b) return  1;
  return 0;
}

static int cmpr_core(const void *a, const void *b) {
  const int _core_a = ULIBC_get_cpuinfo(*(int *)a).core;
  const int _core_b = ULIBC_get_cpuinfo(*(int *)b).core;
  if ( _core_a < _core_b) return -1;
  if ( _core_a > _core_b) return  1;
  return 0;
}

static int cmpr_node(const void *a, const void *b) {
  const int _node_a = ULIBC_get_cpuinfo(*(int *)a).node;
  const int _node_b = ULIBC_get_cpuinfo(*(int *)b).node;
  if ( _node_a < _node_b) return -1;
  if ( _node_a > _node_b) return  1;
  return 0;
}

static int cmpr_smt(const void *a, const void *b) {
  const int _smt_a = ULIBC_get_cpuinfo(*(int *)a).smt;
  const int _smt_b = ULIBC_get_cpuinfo(*(int *)b).smt;
  if ( _smt_a < _smt_b) return -1;
  if ( _smt_a > _smt_b) return  1;
  return 0;
}


/* ------------------------------------------------------------
 * make NUMA layout table
 * ------------------------------------------------------------ */
static int make_numainfo(int *proc_list) {
  int onnodes = 0;
  
  /* detect max. online_nodes/online_cores */
  bitmap_t online[MAX_NODES/64] = {0};
  for (int i = 0; i < ULIBC_get_online_procs(); ++i) {
    struct cpuinfo_t ci = ULIBC_get_cpuinfo( proc_list[i] );
    if ( !ISSET_BITMAP(online, ci.node) ) {
      SET_BITMAP(online, ci.node);
      __online_nodelist[onnodes++] = ci.node;
    }
  }
  
  /* construct numainfo and online_cores */
  for (int i = 0; i < onnodes; ++i) {
    __online_ncores_on_node[i] = 0;
  }
  for (int i = 0; i < ULIBC_get_online_procs(); ++i) {
    int node = 0;
    int cpu = proc_list[i];
    struct cpuinfo_t ci = ULIBC_get_cpuinfo(cpu);
    for (int j = 0; j < onnodes; ++j) {
      if (__online_nodelist[j] == ci.node) node = j;
    }
    __numainfo[i].id   = i;	/* thread index */
    __numainfo[i].proc = cpu;	/* processor index for cpuinfo[] */
    __numainfo[i].node = node;	/* NUMA node index  */
    __numainfo[i].core = __online_ncores_on_node[node]++; /* NUMA core index */
  }
  for (int i = 0; i < ULIBC_get_online_procs(); ++i) {
    __numainfo[i].lnp = __online_ncores_on_node[ __numainfo[i].node ];
  }
  
  return onnodes;
}
