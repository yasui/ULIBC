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

static int __enable_online_procs = 0;	 /* 0: cannot detect online processors, 1: enabled */
static int __max_online_procs;		 /* number of available processors */
static int __online_proclist[MAX_CPUS]; /* online processor indices */
int __detect_external_affinity = 0;

/* thread id */
static int64_t number_of_active_threads = 0;
void ULIBC_clear_thread_num(void) {
  number_of_active_threads = 0;
}

static __thread int __thread_id = -1;
int ULIBC_get_thread_num(void) {
  if (__thread_id < 0) {
    if ( omp_in_parallel() ) {
      __thread_id = omp_get_thread_num();
    } else {
      __thread_id = fetch_and_add_int64(&number_of_active_threads, 1);
    }
  }
  return __thread_id;
}


int get_online_proc_list(int *cpuset);
static int get_string_proc_list(char *string, int *procs);

int ULIBC_init_online_topology(void) {
  double t;
  
  /* make processor list */
  char *proclist_env = getenv("ULIBC_PROCLIST");
  if ( !proclist_env ) {
    PROFILED( t, __max_online_procs = get_online_proc_list(__online_proclist) );
  } else {
    if ( ULIBC_verbose() ) {
      printf("ULIBC: ULIBC_PROCLIST=\"%s\"\n", proclist_env);
    }
    PROFILED( t, __max_online_procs = get_string_proc_list(proclist_env, __online_proclist) );
  }
  
  if (__max_online_procs == 0) {
    __enable_online_procs = 0;
    char proclist[10]="";
    sprintf(proclist, "0-%d", ULIBC_get_num_procs()-1);
    __max_online_procs = get_string_proc_list(proclist, __online_proclist);
  } else {
    __enable_online_procs = 1;
  }
  
  if ( ULIBC_verbose() ) {
    ULIBC_print_main_thread_binding(stdout);
    ULIBC_print_openmp_binding(stdout);
    ULIBC_print_online_topology(stdout);
  }
  
  return 0;
}

/* get functions */
int ULIBC_enable_online_procs(void) { return __enable_online_procs; }

int ULIBC_get_max_online_procs(void) { return __max_online_procs; }
int ULIBC_get_online_procidx(unsigned idx) {
  if ((int)idx > ULIBC_get_max_online_procs())
    idx %= ULIBC_get_max_online_procs();
  return __online_proclist[idx];
}


/* ------------------------------------------------------------ *\
 * Print functions for topology 
 * ------------------------------------------------------------ */
void ULIBC_print_topology(FILE *fp) {
  if (!fp) return;
  
  int ncores_per_socket[MAX_NODES] = { 0 };
  for (int k = 0; k < ULIBC_get_num_nodes(); ++k) {
    char bind_str[MAX_CPUS+1];
    for (int i = 0; i < ULIBC_get_num_procs(); ++i) bind_str[i] = '-';
    bind_str[ ULIBC_get_num_procs() ] = '\0';
    for (int i = 0; i < ULIBC_get_num_procs(); ++i) {
      const int proc = i;
      if ( ULIBC_get_cpuinfo( proc ).node == k ) {
	bind_str[ proc ] = 'x';
	ncores_per_socket[k] ++;
      }
    }
    fprintf(fp, "ULIBC: Package %3d of %d has %d CPUs { %s }\n",
	    k, ULIBC_get_num_nodes(), ncores_per_socket[k], bind_str);
  }
  for (int k = 0; k < ULIBC_get_num_nodes(); ++k) {
    fprintf(fp,
	    "ULIBC: Package %3d of %d has %d CPUs and %.3f GB RAM (%ld-bytes page)\n",
	    k, ULIBC_get_num_nodes(), ncores_per_socket[k],
	    1.0*ULIBC_memory_size(k)/(1UL<<30), ULIBC_page_size(k));
  }
  for (int i = 0; i < ULIBC_get_num_procs(); ++i) {
    struct cpuinfo_t ci = ULIBC_get_cpuinfo(i);
    fprintf(fp, "ULIBC: CPU[%03d] Processor: %2d, Package: %2d, Core: %2d, SMT: %2d\n",
	    i, ci.id, ci.node, ci.core, ci.smt);
  }
}


/* ------------------------------------------------------------ *\
 * Print online topology 
 * ------------------------------------------------------------ */
void ULIBC_print_online_topology(FILE *fp) {
  if ( !fp ) return;
  fprintf(fp, "ULIBC: #online-processors is %d\n",  ULIBC_get_max_online_procs());
  for (int k = 0; k < ULIBC_get_num_nodes(); ++k) {
    fprintf(fp, "ULIBC: Online processors on Package %3d of %d are { ", k, ULIBC_get_num_nodes());
    for (int i = 0; i < ULIBC_get_max_online_procs(); ++i) {
      const int proc = ULIBC_get_online_procidx(i);
      if ( ULIBC_get_cpuinfo( proc ).node == k )
	fprintf(fp, "%d ", proc);
    }
    fprintf(fp, "}\n");
  }
}


/* detects online processor list */
void ULIBC_print_main_thread_binding(FILE *fp) {
  if ( !fp ) return;
  const int ncpus = ULIBC_get_num_procs();
  int bind_nprocs = 0;
  int bind_proc[MAX_CPUS] = { -1 };
  for (int i = 0; i < ncpus; ++i) {
    if ( is_online_proc(i) )
      bind_proc[ bind_nprocs++ ] = i;
  }
  if ( bind_nprocs > 0 ) {
    char bind_str[MAX_CPUS+1];
    for (int j = 0; j < ncpus; ++j) bind_str[j] = '-';
    bind_str[ncpus] = '\0';
    for (int j = 0; j < bind_nprocs; ++j) {
      bind_str[ bind_proc[j] ] = 'x';
    }
    fprintf(fp, "ULIBC: Main Thread       bound to OS CPUs { %s }\n", bind_str);
  }
}

void ULIBC_print_openmp_binding(FILE *fp) {
  if ( !fp ) return;
  const int ncpus = ULIBC_get_num_procs();
  for (int i = 0; i < ncpus; ++i) {
    int bind_nprocs = 0;
    int bind_proc[MAX_CPUS] = { -1 };
    OMP("omp parallel") {
      const int id = ULIBC_get_thread_num();
      if (id == i)
	for (int j = 0; j < ncpus; ++j) {
	  if ( is_online_proc(j) )
	    bind_proc[ bind_nprocs++ ] = j;
	}
    }
    if ( bind_nprocs > 0 ) {
      char bind_str[MAX_CPUS+1];
      for (int j = 0; j < ncpus; ++j) bind_str[j] = '-';
      bind_str[ncpus] = '\0';
      for (int j = 0; j < bind_nprocs; ++j) {
	bind_str[ bind_proc[j] ] = 'x';
      }
      fprintf(fp, "ULIBC: OpenMP Thread %3d bound to OS CPUs { %s }\n", i, bind_str);
    }
  }
}

int get_online_proc_list(int *proc_list) {
  int online_ncpus = 0;
  
  const int ncpus = ULIBC_get_num_procs();
  int bind_thread_count_main[MAX_CPUS];
  int bind_thread_count_omp[MAX_CPUS];
  int bind_proc_count_omp[MAX_CPUS];
  int bind_leading_proc[MAX_CPUS];
  
  for (int i = 0; i < ncpus; ++i) {
    bind_thread_count_main[i] = 0;
    bind_thread_count_omp[i] = 0;
    bind_proc_count_omp[i] = 0;
    bind_leading_proc[i] = -1;
  }
  
  /* main thread */
  for (int i = 0; i < ncpus; ++i) {
    /* processor */
    if ( is_online_proc(i) )
      ++bind_thread_count_main[i];
  }
  
  /* openmp threads */
  OMP("omp parallel") {
    const int id = ULIBC_get_thread_num();
    for (int i = 0; i < ncpus; ++i) {
      if ( is_online_proc(i) ) {
	/* thread */
	++bind_proc_count_omp[id];
	if ( bind_leading_proc[id] < 0 )
	  bind_leading_proc[id] = i;
	/* processor */
      	OMP("omp atomic")
      	  ++bind_thread_count_omp[i];
      }
    }
  }
  
  /* online processor list */
  for (int i = 0; i < ncpus; ++i) {
    if ( bind_thread_count_main[i] > 0 ||
	 bind_thread_count_omp[i] > 0 )
      proc_list[online_ncpus++] = i;
  }
  
  /* sorted */
  int simple_bind_thread_count = 0;
  for (int i = 0; i < online_ncpus; ++i) {
    if ( bind_proc_count_omp[i] == 1 )
      ++simple_bind_thread_count;
  }
  if ( simple_bind_thread_count == online_ncpus ) {
    __detect_external_affinity = 1;
    for (int i = 0; i < ncpus; ++i) {
      proc_list[i] = -1;
    }
    for (int i = 0; i < online_ncpus; ++i) {
      proc_list[i] = bind_leading_proc[i];
    }
  }
  
  return online_ncpus;
}

/* generates online processor list from string */
static int cmpr_int(const void *a, const void *b) {
  const int _a = *(int *)a;
  const int _b = *(int *)b;
  if ( _a < _b) return -1;
  if ( _a > _b) return  1;
  return 0;
}

static int get_string_proc_list(char *string, int *procs) {
  int online = 0;
  const char *sep = ",: ";
  char *comma = NULL;
  for (char *p = strtok_r(string, sep, &comma); p; p = strtok_r(NULL, sep, &comma)) {
    const char *rangesep = "-";
    char *pp = NULL;
    int start = 0, stop = 0;
    if ( (pp = strtok(p, rangesep)) )
      start = stop = atoi(pp);
    else
      goto done;
    if ( (pp = strtok(NULL, rangesep)) )
      stop = atoi(pp);
    else 
      goto done;
    if ( (pp = strtok(NULL, rangesep)) ) {
      if (ULIBC_verbose() > 1)
	printf("[error] wrong processor list\n");
      exit(1);
    }
    if (start > stop) {
      int t = start;
      start = stop, stop = t;
    }
  done:
    if ( start < 0 || stop >= ULIBC_get_num_procs() ) {
      if (ULIBC_verbose() > 1)
	printf("[warning] processor list [%d:%d] skipped\n", start, stop);
      start = stop = -1;
    }
    if (stop < start) {
      if (ULIBC_verbose() > 1)
	printf("[warning] fixed processor list [%d:%d] => ", start, stop);
      int p1 = start, p2 = stop;
      start = p2, stop = p1;
      if (ULIBC_verbose() > 1)
	printf("[%d:%d]\n", start, stop);
    }
    if ( start >= 0 || stop >= 0 )
      for (int i = start; i <= stop; ++i) {
	procs[online++] = i;
      }
  }
  online = uniq(procs, online, sizeof(int), qsort, cmpr_int);
  return online;
}
