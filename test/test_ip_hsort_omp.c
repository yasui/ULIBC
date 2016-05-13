#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <time.h>
#include <inttypes.h>
#include <ulibc.h>
#include <omp_helpers.h>

void partial(int64_t sz, int64_t off, int64_t np, int64_t id, int64_t *ls, int64_t *le) {
  const int64_t qt = sz / np;
  const int64_t rm = sz % np;
  *ls = qt * (id+0) + (id+0 < rm ? id+0 : rm) + off;
  *le = qt * (id+1) + (id+1 < rm ? id+1 : rm) + off;
}

int compar(const void *a, const void *b) {
  const long _a = *(long *)a;
  const long _b = *(long *)b;
  if (_a > _b) return -1;
  if (_a < _b) return  1;
  return 0;
}

int main(int argc, char **argv) {
  ULIBC_init();
  
  long i, cs, len = 100, *a;
  double t1, t2;
  
  if (argc > 1) {
    len = atol(argv[1]);
  }
  setbuf(stdout, NULL);
  
  printf("len  is %ld\n", len);
  printf("\n");
  
  /* random sequence */
  printf("generate %ld elements (%.2f MBytes)\n",
	 len, (double)sizeof(long)*len/(1<<20));
  srand(time(NULL));
  a = malloc(sizeof(long)*len);
  cs = 0;
  srand(time(NULL));
  for (i = 0; i < len; ++i) {
    a[i] = -100 + rand() % (200);
    /* printf("%ld ", a[i]); */
    cs += a[i];
  }
  printf("\n");
  printf("checksum is %ld\n", cs);
  printf("\n");
  
  
  /* heapsort */
  t1 = get_msecs();
  printf("heapsort() ... ");
  printf("\n");
#if 1
  OMP("omp parallel") {
    struct numainfo_t ni = ULIBC_get_current_numainfo();
    int64_t ls, le;
    partial(len, 0, ULIBC_get_num_threads(), ni.id, &ls, &le);
    printf("[%8" PRId64 ",%8" PRId64 "] => %" PRId64 ": %d of %d threads\n",
	   ls, le, le-ls, ni.id, ULIBC_get_num_threads());
    uheapsort(&a[ls], le-ls, sizeof(long), compar);
  }
#else
  uheapsort(a, len, sizeof(long), compar);
#endif
  t2 = get_msecs();
  printf("(%.2f msec.)\n", t2-t1);
  printf("\n");
  
  printf("verification\n");
  for (i = 0; i < len; ++i) {
    /* printf("%ld ", a[i]); */
    cs -= a[i];
  }
  printf("\n");
  printf("checksum diff is %ld\n", cs);
  printf("\n");
  
  
  /* uniq */
  long nr = 0;
  t1 = get_msecs();
  printf("uniq() ... ");
  nr = uniq(a, len, sizeof(long), qsort, compar);
  t2 = get_msecs();
  printf("(%.2f msec.)\n", t2-t1);
  printf("\n");
  printf("verification(%ld elements)\n", nr);
  for (i = 0; i < nr; ++i) {
    /* printf("%ld ", a[i]); */
  }
  printf("\n");
  
  free(a);
  
  return 0;
}
