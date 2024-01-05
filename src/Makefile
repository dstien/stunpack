SUBDIRS = lib

BIN = stunpack$(EXESUFFIX)
SRCS = main.c
OBJS = $(SRCS:%.c=$(BUILDDIR)/%.o)
LIBS = $(BUILDDIR)/lib/libstunpack$(LIBSUFFIX)

# Watcom linker expects libs before objects
ifneq (,$(findstring wc,$(firstword $(CC))))
	LINK_INPUTS = $(LIBS) $(OBJS)
else
	LINK_INPUTS = $(OBJS) $(LIBS)
endif

all: $(BUILDDIR)/$(BIN)

$(BUILDDIR)/$(BIN): $(LINK_INPUTS)
	$(CC) $(LDFLAGS)$@ $^

$(BUILDDIR)/%.o: %.c
	$(CC) $(CFLAGS)$@ $<

subdirs: $(SUBDIRS)

$(SUBDIRS):
	test -d "$(BUILDDIR)/$@" || mkdir -p "$(BUILDDIR)/$@"
	$(MAKE) -C $@ BUILDDIR="../$(BUILDDIR)/$@" $(MAKECMDGOALS)

$(LIBS): subdirs

clean: subdirs
	rm -f "$(BUILDDIR)"/*.o "$(BUILDDIR)"/*.err "$(BUILDDIR)/$(BIN)"

install: $(BUILDDIR)/$(BIN)
	install -m 755 "$<" "$(INSTALLDIR)"

uninstall:
	rm -f "$(INSTALLDIR)/$(BIN)"

.PHONY: all clean install uninstall subdirs $(SUBDIRS)
