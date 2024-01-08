BIN = libstunpack$(LIBSUFFIX)
SRCS = stunpack.c stunts.c stunts_huff.c stunts_rle.c util.c
OBJS = $(SRCS:%.c=$(BUILDDIR)/%.o)

all: $(BUILDDIR)/$(BIN)

$(BUILDDIR)/$(BIN): $(OBJS)
	$(AR) $(ARFLAGS) $@ $(OBJS)

$(BUILDDIR)/%.o: %.c
	$(CC) $(CFLAGS)$@ $<

clean:
	rm -f "$(BUILDDIR)"/*.o "$(BUILDDIR)"/*.err "$(BUILDDIR)/$(BIN)"

.PHONY: all clean install uninstall
