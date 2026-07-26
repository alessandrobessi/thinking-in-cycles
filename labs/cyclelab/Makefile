# Builds cyclelab in two variants, both with debug symbols retained
# (BLUEPRINT.md Section 6/13.1: "built with debug symbols and optimized
# code" / "debug symbols retained for profiling"):
#
#   make debug    -> bin/cyclelab-debug   (-O0 -g, for teaching/stepping)
#   make release  -> bin/cyclelab         (-O2 -g, for the actual labs)
#
# `make` alone builds both.

CC ?= cc
STD := -std=c11
# glibc hides POSIX.1-2008 declarations (clock_gettime/CLOCK_MONOTONIC,
# gmtime_r, nanosleep) behind a feature-test macro under strict -std=c11.
# _DEFAULT_SOURCE is glibc's own recommended macro for "expose
# POSIX+BSD+SVID as glibc's normal default" -- unlike _POSIX_C_SOURCE,
# it does NOT additionally *restrict* anything, which matters here:
# macOS's libc doesn't recognize _DEFAULT_SOURCE at all, so it's a
# harmless no-op there, leaving Darwin's own permissive default (which
# is what exposes BSD-only struct rusage fields like ru_nvcsw/ru_nivcsw,
# and _SC_NPROCESSORS_ONLN) untouched. _POSIX_C_SOURCE was tried first
# and broke the macOS build for exactly this reason -- it does
# additionally restrict on Darwin, unlike on Linux.
POSIX := -D_DEFAULT_SOURCE
WARN := -Wall -Wextra
LDFLAGS := -lpthread

DEBUG_CFLAGS := $(STD) $(POSIX) $(WARN) -O0 -g
RELEASE_CFLAGS := $(STD) $(POSIX) $(WARN) -O2 -g

BINDIR := bin
SRC := $(wildcard src/*.c) $(wildcard src/modes/*.c)

.PHONY: all debug release clean

all: debug release

debug: $(BINDIR)/cyclelab-debug

release: $(BINDIR)/cyclelab

$(BINDIR):
	mkdir -p $(BINDIR)

$(BINDIR)/cyclelab-debug: $(SRC) | $(BINDIR)
	$(CC) $(DEBUG_CFLAGS) -DCYCLELAB_BUILD_TYPE='"debug"' -DCYCLELAB_CFLAGS='"$(DEBUG_CFLAGS)"' \
		-o $@ $(SRC) $(LDFLAGS)

$(BINDIR)/cyclelab: $(SRC) | $(BINDIR)
	$(CC) $(RELEASE_CFLAGS) -DCYCLELAB_BUILD_TYPE='"release"' -DCYCLELAB_CFLAGS='"$(RELEASE_CFLAGS)"' \
		-o $@ $(SRC) $(LDFLAGS)

clean:
	rm -rf $(BINDIR)
