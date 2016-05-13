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

#include <ulibc.h>
#include <common.h>

/* ------------------------------------------------------------
 * allocations
 * ------------------------------------------------------------ */
char *ULIBC_get_memory_name(void) {
#if defined(USE_MALLOC)
  return "malloc";
#else
  return "posix_memalign";
#endif
}

void *NUMA_malloc(size_t size, const int onnode) {
  if (size == 0)
    return NULL;
  
  const int node = ULIBC_get_online_nodeidx(onnode);
  if (ULIBC_verbose() > 1)
    printf("ULIBC: NUMA %d on socket %d allocates %ld Bytes\n", onnode, node, size);
  
  void *p = NULL;
#ifdef USE_MALLOC
  p = malloc(size);
#else
  posix_memalign((void *)&p, ULIBC_align_size(), size);
#endif
  return p;
}


void *NUMA_touched_malloc(size_t sz, int onnode) {
  void *p = NUMA_malloc(sz, onnode);

  OMP("omp parallel") {
    struct numainfo_t ni = ULIBC_get_current_numainfo();
    if ( onnode == ni.node ) {
      const long pgsz = ULIBC_page_size(ni.node);
      unsigned char *x = p;
      long ls, le;
      prange(sz, 0, ni.lnp, ni.core, &ls, &le);
      for (long k = ls; k < le; k += pgsz) x[k] = -1;
    }
  }
  
  return p;
}

void ULIBC_touch_memories(size_t size[MAX_NODES], void *pool[MAX_NODES]) {
  if (!pool) {
    fprintf(stderr, "ULIBC: Wrong address: pool is %p\n", pool);
    return;
  }
  OMP("omp parallel") {
    struct numainfo_t ni = ULIBC_get_current_numainfo();
    unsigned char *addr = pool[ni.node];
    const size_t sz = size[ni.node];
    if (addr) {
      long ls, le;
      const int lnp = ULIBC_get_online_cores( ni.node );
      const long stride = ULIBC_page_size(ni.node);
      prange(sz, 0, lnp, ni.core, &ls, &le);
      for (long k = ls; k < le; k += stride)
	addr[k] = ~0;
    }
  }
}

void NUMA_free(void *p) {
  if (p) free(p);
}
