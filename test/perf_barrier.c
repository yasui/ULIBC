#include <stdio.h>
#include <stdlib.h>
#include <ulibc.h>
#include <omp_helpers.h>

int main(int argc, char **argv) {
  ULIBC_init();
  
  (void)argc;
  (void)argv;
  
  int iterations;
  double t1, t2;
  double ulibc_bind_ms, omp_barrier_ms, ulibc_barrier_ms, ulibc_node_barrier_ms, ulibc_hier_barrier_ms;
  
  /* omp region */
  iterations = 10000;
  t1 = omp_get_wtime();
  for (int i = 0; i < iterations; ++i) {
    OMP("omp parallel") {
      struct numainfo_t loc = ULIBC_get_current_numainfo();
      (void)loc;
    }
  }
  t2 = omp_get_wtime();
  ulibc_bind_ms = (t2-t1) * 1000 / iterations;
  
  /* barrier */
  iterations = 10000;
  t1 = omp_get_wtime();
  OMP("omp parallel") {
    struct numainfo_t loc = ULIBC_get_current_numainfo();
    (void)loc;
    for (int i = 0; i < iterations; ++i) {
      OMP("omp barrier");
    }
  }
  t2 = omp_get_wtime();
  omp_barrier_ms = ( (t2-t1) - ulibc_bind_ms ) / iterations * 1000.0;

  /* ULIBC_barrier */
  iterations = 1000;
  t1 = omp_get_wtime();
  OMP("omp parallel") {
    struct numainfo_t loc = ULIBC_get_current_numainfo();
    (void)loc;
    for (int i = 0; i < iterations; ++i) {
      ULIBC_barrier();
    }
  }
  t2 = omp_get_wtime();
  ulibc_barrier_ms = ( (t2-t1) - ulibc_bind_ms ) / iterations * 1000.0;

  /* ULIBC_node_barrier */
  iterations = 100000;
  t1 = omp_get_wtime();
  OMP("omp parallel") {
    struct numainfo_t loc = ULIBC_get_current_numainfo();
    (void)loc;
    for (int i = 0; i < iterations; ++i) {
      ULIBC_node_barrier();
    }
  }
  t2 = omp_get_wtime();
  ulibc_node_barrier_ms = ( (t2-t1) - ulibc_bind_ms ) / iterations * 1000.0;

  /* ULIBC_hierarchical_barrier */
  iterations = 100000;
  t1 = omp_get_wtime();
  OMP("omp parallel") {
    struct numainfo_t loc = ULIBC_get_current_numainfo();
    (void)loc;
    for (int i = 0; i < iterations; ++i) {
      ULIBC_hierarchical_barrier();
    }
  }
  t2 = omp_get_wtime();
  ulibc_hier_barrier_ms = ( (t2-t1) - ulibc_bind_ms ) / iterations * 1000.0;
  
  printf("%2s %8s %8s %8s %8s %8s\n", "np",
	 "omp_U", "omp_brr", "U_brr", "Und_brr", "Uhr_brr");
  
  printf("%2d %8f %8f %8f %8f %8f\n", omp_get_max_threads(), ulibc_bind_ms, omp_barrier_ms, ulibc_barrier_ms, ulibc_node_barrier_ms, ulibc_hier_barrier_ms);
  
  return 0;
}
