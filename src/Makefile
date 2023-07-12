BIN = stunpack$(EXESUFFIX)
INSTALLDIR ?= /usr/local/bin

SRCS = stunpack.c main.c
OBJS = $(SRCS:%.c=$(BUILDDIR)/%.o)

all: $(BUILDDIR)/$(BIN)

$(BUILDDIR)/$(BIN): $(OBJS)
	$(CC) $(LDFLAGS)$@ $(OBJS)

$(BUILDDIR)/%.o: %.c
	$(CC) $(CFLAGS)$@ $<

clean:
	rm -f "$(BUILDDIR)"/*.o "$(BUILDDIR)"/*.err "$(BUILDDIR)/$(BIN)"

install: $(BUILDDIR)/$(BIN)
	install -m 755 "$(BUILDDIR)/$(BIN)" "$(INSTALLDIR)"

uninstall:
	rm -f "$(INSTALLDIR)/$(BIN)"

.PHONY: all clean install uninstall
