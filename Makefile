include config.mak

LIBTEST = test-libdlna
SRCS = test-libdlna.c

CFLAGS += -Isrc
LDFLAGS += -Lsrc -ldlna

all: lib test

lib:
	$(MAKE) -C src

test: $(LIBTEST)

$(LIBTEST): $(SRCS)
	$(CC) $? $(OPTFLAGS) $(CFLAGS) $(LDFLAGS) -o $@

clean:
	$(MAKE) -C src clean
	-$(RM) -f $(LIBTEST)

distclean: clean
	-$(RM) -f config.log
	-$(RM) -f config.mak

install:
	$(MAKE) -C src install

.PHONY: clean distclean
