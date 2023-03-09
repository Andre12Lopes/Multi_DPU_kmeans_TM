ROOT = .

include Makefile.common

CC = dpu-upmem-dpurte-clang

DIRS = kmeans host

.PHONY:	all clean $(DIRS)

all: $(LIBNOREC)

%.o: %.c
	$(CC) $(CFLAGS) $(DEFINES) -c -o $@ $<

# Additional dependencies
$(SRCDIR)/norec.o: $(SRCDIR)/norec.h $(SRCDIR)/thread_def.h $(SRCDIR)/utils.h


$(LIBNOREC): $(SRCDIR)/$(TM).o
	$(AR) crus $@ $^

test: $(LIBNOREC) $(DIRS)

$(DIRS):
	$(MAKE) -C $@

clean:
	rm -f $(LIBNOREC) $(SRCDIR)/*.o
	$(MAKE) -C kmeans clean
	$(MAKE) -C host clean
