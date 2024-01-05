BUILDDIR ?= .

SUBDIRS = src

# Default GNU tool chain options
CFLAGS += -c -O2 -Wall -I"$(CURDIR)/include" -o 
LDFLAGS += -o 

EXESUFFIX ?=
LIBSUFFIX ?= .a
INSTALLDIR ?= /usr/local/bin

# Detect Watcom compiler
ifneq (,$(findstring wc,$(firstword $(CC))))
	CFLAGS = -d0 -ox -zastd=c99 -aa -zq -c -i="$(CURDIR)/include" -fo=
	LDFLAGS = -zq -l=pmodew -fe=
	AR = wlib
	ARFLAGS = -q
	EXESUFFIX = .exe
	LIBSUFFIX = .lib
# Detect Mingw compiler
else ifneq (,$(findstring mingw,$(firstword $(CC))))
	EXESUFFIX = .exe
# Detect Zig cc for Windows
else ifneq (,$(findstring windows,$(CC)))
	EXESUFFIX = .exe
endif

export CC CFLAGS LDFLAGS AR ARFLAGS EXESUFFIX LIBSUFFIX INSTALLDIR

subdirs: $(SUBDIRS)

$(SUBDIRS):
	test -d "$(BUILDDIR)/$@" || mkdir -p "$(BUILDDIR)/$@"
	$(MAKE) -C $@ BUILDDIR="../$(BUILDDIR)/$@" $(MAKECMDGOALS)

all clean install uninstall: subdirs

.PHONY: all clean install uninstall subdirs $(SUBDIRS)
