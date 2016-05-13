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
#include <ulibc.h>
#include <common.h>

int ULIBC_init_numa_threads(void) {
  if ( ULIBC_use_affinity() == NULL_AFFINITY ) return 1;
  if ( ULIBC_verbose() ) {
    ULIBC_print_thread_mapping(stdout);
    ULIBC_print_main_thread_binding(stdout);
    ULIBC_print_openmp_binding(stdout);
  }
  return 0;
}

int ULIBC_bind_thread_explicit(int threadid) {
  (void)threadid;
  if ( ULIBC_use_affinity() != ULIBC_AFFINITY ) return 0;
  return 1;
}

int ULIBC_bind_thread(void) {
  if ( ULIBC_use_affinity() != ULIBC_AFFINITY ) return 0;
  return 1;
}

int ULIBC_unbind_thread(void) {
  if ( ULIBC_use_affinity() != ULIBC_AFFINITY ) return 0;
  return 1;
}

int ULIBC_is_bind_thread(int proc) {
  (void)proc;
  return 0;
}
