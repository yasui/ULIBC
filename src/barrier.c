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
#include <unistd.h>
#include <pthread.h>

#if _POSIX_BARRIERS < 0
#include <pthread_barrier_emu.h>
#endif

#include <ulibc.h>
#include <common.h>

static pthread_barrier_t __all_barrier;
static pthread_barrier_t __node_master_barrier;
static pthread_barrier_t __pair_wise_barrier[ MAX_NODES ][ MAX_NODES ];

int ULIBC_init_barriers(void) {
  /* all */
  pthread_barrier_init(&__all_barrier, NULL, ULIBC_get_online_procs());
  
  /* node master */
  pthread_barrier_init(&__node_master_barrier, NULL, ULIBC_get_online_nodes());
  
  /* pairs */
  for (int node_s = 0; node_s < ULIBC_get_online_nodes(); ++node_s) {
    for (int node_t = 0; node_t < ULIBC_get_online_nodes(); ++node_t) {
      const int node_s_lnp = ULIBC_get_online_cores(node_s);
      const int node_t_lnp = ULIBC_get_online_cores(node_t);
      pthread_barrier_init(&__pair_wise_barrier[node_s][node_t], NULL, node_s_lnp+node_t_lnp);
      pthread_barrier_init(&__pair_wise_barrier[node_t][node_s], NULL, node_s_lnp+node_t_lnp);
    }
  }
  return 0;
}

void ULIBC_barrier(void) {
  pthread_barrier_wait( &__all_barrier );
}

void ULIBC_pair_barrier(int node_s, int node_t) {
  const int min_node = MIN(node_s,node_t);
  const int max_node = MAX(node_s,node_t);
  pthread_barrier_wait( & __pair_wise_barrier[min_node][max_node] );
}

void ULIBC_hierarchical_barrier(void) {
  const int core = ULIBC_get_numainfo( ULIBC_get_thread_num() ).core;
  ULIBC_node_barrier();
  if ( core == 0 ) {
    pthread_barrier_wait( & __node_master_barrier );
  }
  ULIBC_node_barrier();
}
