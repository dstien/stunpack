BUILDDIR ?= .

SUBDIRS = src

# Default GNU tool chain options
CC ?= gcc
CFLAGS += -c -O2 -Wall -o 
LDFLAGS += -o 
EXESUFFIX ?=

# Detect Watcom compiler
ifneq (,$(findstring wc,$(firstword $(CC))))
	CFLAGS = -d0 -ox -za99 -zq -c -fo=
	LDFLAGS = -zq -l=pmodew -fe=
	EXESUFFIX = .exe
# Detect Mingw compiler
else ifneq (,$(findstring mingw,$(firstword $(CC))))
	EXESUFFIX = .exe
# Detect Zig cc for Windows
else ifneq (,$(findstring windows,$(CC)))
	EXESUFFIX = .exe
endif

export CC CFLAGS LDFLAGS EXESUFFIX

all clean install uninstall:
	@for dir in $(SUBDIRS); do \
		test -d "$(BUILDDIR)/$$dir" || mkdir -p "$(BUILDDIR)/$$dir"; \
		$(MAKE) -C $$dir BUILDDIR="../$(BUILDDIR)/$$dir" $@; \
	done

.PHONY: all clean install uninstall
