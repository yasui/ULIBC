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

static int  __verbose = 0;
int ULIBC_verbose(void) { return __verbose; }

int ULIBC_init(void) {
  /* sets parameter from environment variables. */
  __verbose = getenvi("ULIBC_VERBOSE", 0);

  ULIBC_set_version();
  if (ULIBC_verbose()) printf("ULIBC: version %s\n", ULIBC_version());
  
  int ret = 0;
  TOPLEVEL_PROFILED( ret |= ULIBC_init_topology() );
  TOPLEVEL_PROFILED( ret |= ULIBC_init_online_topology() );
  TOPLEVEL_PROFILED( ret |= ULIBC_init_numa_mapping() );
  TOPLEVEL_PROFILED( ret |= ULIBC_init_numa_threads() );
  TOPLEVEL_PROFILED( ret |= ULIBC_init_numa_barriers() );
  TOPLEVEL_PROFILED( ret |= ULIBC_init_barriers() );
  TOPLEVEL_PROFILED( ret |= ULIBC_init_numa_loops() );
  
  if (ULIBC_verbose()) printf("ULIBC: successfully finished.\n");
  return ret;
}
