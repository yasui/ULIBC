#include <stdio.h>
#include <assert.h>
#include <ulibc.h>
#include <omp_helpers.h>

#ifndef TIMED
#  define TIMED(t, X) do { double tt=get_msecs(); X; t=get_msecs()-tt; } while (0)
#endif

int main(int argc, char **argv) {
  ULIBC_init();
  
  int scale = 10, chunk = 128;
  printf("usage: ./%s SCALE chunk\n", argv[0]);
  if (argc > 1) {
    --argc, ++argv;
    scale = atof(*argv);
  }
  if (argc > 1) {
    --argc, ++argv;
    chunk = atof(*argv);
  }
  
  int64_t static_loop(int64_t n);
  int64_t dynamic_loop(int64_t n, int64_t chunk);
  
  printf("#procs   is %d\n", ULIBC_get_num_procs());
  printf("#nodes   is %d\n", ULIBC_get_num_nodes());
  printf("#cores   is %d\n", ULIBC_get_num_cores());
  printf("#smt     is %d\n", ULIBC_get_num_smts());
  printf("\n");
  
  int64_t n = 1ULL << scale;
  printf("n is %lld (SCALE: %d)\n", (long long)n, scale);
  int64_t total = n*(n+1)/2;
  printf("total is %lld\n", (long long)total);
  printf("\n");
  
  double s_time, d_time;
  int64_t total_static, total_dynamic;
  TIMED( s_time, total_static  = static_loop(n) );
  TIMED( d_time, total_dynamic = dynamic_loop(n, chunk) );
  
  printf("\n");
  printf("total_static  is %lld (%.3f ms)\n", (long long)total_static, s_time);
  printf("total_dynamic is %lld (chunk: %d) (%.3f ms)\n",
	 (long long)total_dynamic, chunk, d_time);
  assert( total_static == total );
  assert( total_dynamic == total );
  
  return 0;
}

void range(int64_t len, int64_t off, int64_t np, int64_t id, int64_t *ls, int64_t *le);

int64_t static_loop(int64_t n) {
  int64_t total = 0;
  OMP("omp parallel reduction(+:total)") {
    struct numainfo_t ni = ULIBC_get_current_numainfo();
    int64_t node_ls, node_le;
    range(n+1, 0, ULIBC_get_online_nodes(), ni.node, &node_ls, &node_le);
    int64_t ls, le;
    int lnp = ULIBC_get_online_cores( ni.node );
    range(node_le-node_ls, node_ls, lnp, ni.core, &ls, &le);
    for (int64_t i = ls; i < le; ++i) {
      total += i;
    }
  }
  return total;
}

int64_t dynamic_loop(int64_t n, int64_t chunk) {
  int64_t total = 0;
  OMP("omp parallel reduction(+:total)") {
    struct numainfo_t ni = ULIBC_get_current_numainfo();
    int64_t node_ls, node_le;
    range(n+1, 0, ULIBC_get_online_nodes(), ni.node, &node_ls, &node_le);
    ULIBC_clear_numa_loop(node_ls, node_le);
    ULIBC_node_barrier();
    
    int64_t ls, le;
    while ( !ULIBC_numa_loop(chunk, &ls, &le) ) {
      for (int64_t i = ls; i < le; ++i) {
	total += i;
      }
    }
  }  
  return total;
}

void range(int64_t len, int64_t off, int64_t np, int64_t id, int64_t *ls, int64_t *le) {
  const int64_t qt = len / np;
  const int64_t rm = len % np;
  *ls = qt * (id+0) + (id+0 < rm ? id+0 : rm) + off;
  *le = qt * (id+1) + (id+1 < rm ? id+1 : rm) + off;
}
