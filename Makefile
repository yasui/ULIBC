-include make.rule

OSSPEC_OBJ := topology.o numa_malloc.o numa_threads.o
COMMON_OBJ := init.o online_topology.o numa_mapping.o numa_loops.o barrier.o tools.o

ifeq ($(USE_PTHREAD_BARRIER), yes)
COMMON_OBJ += numa_barrier.o
else
COMMON_OBJ += numa_barrier_opt.o
endif

OBJECTS := $(addprefix $(OBJDIR)/$(OSSPEC)_, $(OSSPEC_OBJ)) $(addprefix $(OBJDIR)/, $(COMMON_OBJ))

.PHONY: all ulibc hwloc clean update_make_headers
all: ulibc

hwloc: $(HWLOC_DIR)
$(HWLOC_DIR): $(HWLOC_TAR)
	@-(! test -e $(HWLOC_DIR)) && tar xvf $(HWLOC_TAR)
	( cd $(HWLOC_DIR) && ./configure $(HWLOC_CONFIG) && make all && make install )
$(HWLOC_TAR): $(HWLOC_TGZ)
	gunzip -c $(HWLOC_TGZ) > $(HWLOC_TAR)
$(HWLOC_TGZ):
	wget $(HWLOC_URL)

ulibc: hwloc $(LIBNAME) $(SOLIBNAME)

$(LIBNAME): $(OBJECTS)
	$(RM) $@
	$(AR) $@ $^
ifdef RANLIB
	$(RANLIB) $@
endif

$(SOLIBNAME): $(OBJECTS)
	$(RM) $@
	$(CC) $(SOFLAGS) $(LDFLAGS) $(OBJECTS) $(SOLDLIBS) -o $@

SUFFIXES: .c .o .h

$(OBJDIR)/%.o: $(SRCDIR)/%.c
	$(CC) $(CFLAGS) $(CPPFLAGS) -c -o $@ $<


test: ulibc install $(TSTDIR)/$(wildcard *.c)
	(cd $(TSTDIR) && $(MAKE))

update_make_headers:
	$(CC) -MM $(CPPFLAGS) $(SRCDIR)/*.c > make.headers
-include make.headers


install: ulibc
	$(MKDIR) $(PREFIX)
	$(MKDIR) $(PREFIX)/lib
	$(MKDIR) $(PREFIX)/include
	$(CP) include/* $(PREFIX)/include
	$(CP) lib/* $(PREFIX)/lib

uninstall:
	$(RM) $(PREFIX)/include/$(LIBINC)
	$(RM) $(PREFIX)/lib/$(LIBNAME)

clean:
	$(RM) *~ \#* $(OBJECTS) gmon.out $(LIBNAME) $(SOLIBNAME)
	$(MAKE) -C $(TSTDIR) clean

### Local variables:
### mode: makefile-bsdmake
### coding: utf-8-unix
### indent-tabs-mode: nil
### End:
