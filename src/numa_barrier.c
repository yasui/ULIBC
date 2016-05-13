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
#include <unistd.h>
#include <pthread.h>

#include <ulibc.h>
#include <common.h>

#if _POSIX_BARRIERS < 0
#include <pthread_barrier_emu.h>
#endif

static pthread_barrier_t __numa_barrier[MAX_NODES];

int ULIBC_init_numa_barriers(void) {
  if ( ULIBC_verbose() ) {
    printf("ULIBC: _POSIX_BARRIERS=%d\n", (int)_POSIX_BARRIERS);
    if (_POSIX_BARRIERS < 0) {
      printf("ULIBC: Using ULIBC Node barrier based on pthread_barrier(emu)\n");
    } else {
      printf("ULIBC: Using ULIBC Node barrier based on pthread_barrier\n");
    }
  }
  for (int k = 0; k < ULIBC_get_online_nodes(); ++k) {
    pthread_barrier_init( &__numa_barrier[k], NULL, ULIBC_get_online_cores(k) );
  }
  
  return 0;
}

void ULIBC_node_barrier(void) {
  const int node = ULIBC_get_numainfo( ULIBC_get_thread_num() ).node;
  pthread_barrier_wait( &__numa_barrier[node] );
}
