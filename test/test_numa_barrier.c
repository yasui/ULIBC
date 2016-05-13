#include <stdio.h>
#include <stdlib.h>
#include <ulibc.h>
#include <omp_helpers.h>

int main(int argc, char **argv) {
  ULIBC_init();
  
  int monitor_nodeid = 0;
  if ( argc == 2 ) {
    monitor_nodeid = atoi(argv[1]);
  } else {
    printf("Usage:\n"
	   "OMP_NUM_THREADS=<# of threads> ./test_numa_barrier monitor_nodeid\n");
    return 0;
  }
  printf("# of threads is %d, monitor_nodeid is %d\n", omp_get_max_threads(), monitor_nodeid);
  
  OMP("omp parallel") {
    struct numainfo_t ni = ULIBC_get_current_numainfo();
    
    double t1, t2;
    t1 = omp_get_wtime();
    if ( ni.node == monitor_nodeid ) {
      ULIBC_node_barrier();
      printf("# [Node %3d on Socket %3d of %3d]"
	     " Hello World from thread %3d of %3d.\n",
	     ni.node, ULIBC_get_online_nodeidx(ni.node),
	     ULIBC_get_online_nodes(),
	     omp_get_thread_num(), omp_get_num_threads());
      ULIBC_node_barrier();
      printf("@ [Node %3d on Socket %3d of %3d]"
	     " Hello World from thread %3d of %3d.\n",
	     ni.node, ULIBC_get_online_nodeidx(ni.node),
	     ULIBC_get_online_nodes(),
	     omp_get_thread_num(), omp_get_num_threads());
      ULIBC_node_barrier();
    }
    t2 = omp_get_wtime();
    OMP("omp barrier");

    if ( ni.node == monitor_nodeid ) {
      printf("[Node %3d on Socket %3d of %3d]"
	     " Time spent in barrier by thread %3d is %f s\n",
	     ni.node, ULIBC_get_online_nodeidx(ni.node),
	     ULIBC_get_online_nodes(),
	     omp_get_thread_num(), t2-t1 );
    }
  }
  
  return 0;
}
