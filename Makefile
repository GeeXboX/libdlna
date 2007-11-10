ifeq (,$(wildcard config.mak))
$(error "config.mak is not present, run configure !")
endif
include config.mak

DISTFILE = libdlna-$(VERSION).tar.bz2

LIBTEST = test-libdlna
SRCS = test-libdlna.c

EXTRADIST = AUTHORS \
	ChangeLog \
	configure \
	COPYING \
	README \

SUBDIRS = src \
	web \

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

dist:
	-$(RM) $(DISTFILE)
	dist=$(shell pwd)/libdlna-$(VERSION) && \
	for subdir in . $(SUBDIRS); do \
		mkdir -p "$$dist/$$subdir"; \
		$(MAKE) -C $$subdir dist-all DIST="$$dist/$$subdir"; \
	done && \
	tar cjf $(DISTFILE) libdlna-$(VERSION)
	-$(RM) -rf libdlna-$(VERSION)

dist-all:
	cp $(EXTRADIST) $(SRCS) Makefile $(DIST)

.PHONY: dist dist-all
