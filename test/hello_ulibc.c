#include <stdio.h>
#include <string.h>
#include <ulibc.h>
#include <sys/resource.h>

size_t ram_usage_bytes(void) {
  struct rusage ru;
  memset(&ru, 0x00, sizeof(struct rusage));
  getrusage(RUSAGE_SELF, &ru);
#if defined(__APPLE__) && defined(__MACH__)
  return ru.ru_maxrss;
#else
  return ru.ru_maxrss * (1UL << 10);
#endif
}

int main(void) {
  ULIBC_init();
  printf("Hello %s!\n", ULIBC_version());
  
  const size_t bytes = ram_usage_bytes();
  const double MB = (double)bytes / (1UL << 20);
  printf("RamUsage: %f MB (%lu bytes)\n", MB, bytes);
  
  return 0;
}
