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
WARN := -Wall -Wextra
LDFLAGS := -lpthread

DEBUG_CFLAGS := $(STD) $(WARN) -O0 -g
RELEASE_CFLAGS := $(STD) $(WARN) -O2 -g

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
