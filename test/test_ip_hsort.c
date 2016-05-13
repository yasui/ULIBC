#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <sys/time.h>
#include <time.h>
#include <ulibc.h>

int compar(const void *a, const void *b) {
  const long _a = *(long *)a;
  const long _b = *(long *)b;
  if (_a > _b) return -1;
  if (_a < _b) return  1;
  return 0;
}

int main(int argc, char **argv) {
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
  uheapsort(a, len, sizeof(long), compar);
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
