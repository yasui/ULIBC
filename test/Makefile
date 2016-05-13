PREFIX   := $(HOME)/local
SOURCES  := $(wildcard *.c)
TARGET   := $(subst .c,, $(SOURCES))
CC       := gcc
CFLAGS   := -O2 -g3 -fopenmp -Wall -Wextra -std=c99 -D_GNU_SOURCE
LDLIBS   := -lulibc -lm
CPPFLAGS := $(addprefix -I, $(PREFIX)/include)

ifeq ($(shell uname -s), Darwin)
LDFLAGS  := $(addprefix -L, $(PREFIX)/lib) -Wl,-rpath,$(PREFIX)/lib
else
LDFLAGS  := $(addprefix -L, $(PREFIX)/lib) -Wl,-no-as-needed -Wl,-R$(PREFIX)/lib -Wl,-rpath-link $(PREFIX)/lib
endif

.PHONY: all clean test
all: test

test: $(TARGET)

clean:
	$(RM) *~ \#* *.o gmon.out $(TARGET)

### Local variables:
### mode: makefile-bsdmake
### coding: utf-8-unix
### indent-tabs-mode: nil
### End:
