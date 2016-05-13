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
  
  OMP("omp parallel") {
    struct numainfo_t loc = ULIBC_get_numainfo( omp_get_thread_num() );
    ULIBC_bind_thread_explicit( loc.id );
  }

  return 0;
}
