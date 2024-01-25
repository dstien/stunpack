BIN = libstunpack$(LIBSUFFIX)
SRCS = dsi.c dsi_huff.c dsi_rle.c rpck.c stunpack.c util.c
OBJS = $(SRCS:%.c=$(BUILDDIR)/%.o)

all: $(BUILDDIR)/$(BIN)

$(BUILDDIR)/$(BIN): $(OBJS)
	$(AR) $(ARFLAGS) $@ $(OBJS)

$(BUILDDIR)/%.o: %.c
	$(CC) $(CFLAGS)$@ $<

clean:
	rm -f "$(BUILDDIR)"/*.o "$(BUILDDIR)"/*.err "$(BUILDDIR)/$(BIN)"

.PHONY: all clean install uninstall
