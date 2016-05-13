#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <ulibc.h>
#include <omp_helpers.h>

int main(void) {
  ULIBC_init();
  
  if ( ULIBC_verbose() < 3) {
    printf("Execute with ULIBC_VERBOSE=2 for more details\n");
    printf("\n");
  }
  printf("#procs   is %d\n", ULIBC_get_num_procs());
  printf("#nodes   is %d\n", ULIBC_get_num_nodes());
  printf("#cores   is %d\n", ULIBC_get_num_cores());
  printf("#smt     is %d\n", ULIBC_get_num_smts());
  printf("\n");
  printf("#threads is %d\n", ULIBC_get_num_threads());
  printf("\n");
  printf("default  is %s-%s\n",
	 ULIBC_get_current_mapping_name(), ULIBC_get_current_binding_name());

  printf("\n");
  const int a_policy[] = { SCATTER_MAPPING, COMPACT_MAPPING, -1 };
  const int b_policy[] = { THREAD_TO_THREAD, THREAD_TO_CORE, THREAD_TO_SOCKET, -1 };
  void test(void);
  for (int i = 0; a_policy[i] >= 0; ++i) {
    for (int j = 0; b_policy[j] >= 0; ++j) {
      printf("c ----------------------------------------\n");
      printf("c %s-%s\n",
      	     ULIBC_get_current_mapping_name(),
      	     ULIBC_get_current_binding_name());
      printf("c ----------------------------------------\n");
      ULIBC_set_affinity_policy(ULIBC_get_num_procs(), a_policy[i], b_policy[j]);
      test();
    }
  }
  return 0;
}

void test(void) {
  for (int i = 0; i < ULIBC_get_online_procs(); ++i) {
    struct numainfo_t ni = ULIBC_get_numainfo(i);
    struct cpuinfo_t ci = ULIBC_get_cpuinfo( ni.proc );
    printf("Thread: %3d of %d, NUMA: %2d-%02d (core=%2d)"
  	   ", Proc: %2d, Pkg: %2d, Core: %2d, Smt: %2d\n",
  	   ni.id, ULIBC_get_online_procs(), ni.node, ni.core, ni.lnp,
  	   ci.id, ci.node, ci.core, ci.smt);
  }
}
