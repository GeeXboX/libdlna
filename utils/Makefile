ifeq (,$(wildcard ../config.mak))
$(error "config.mak is not present, run configure !")
endif
include ../config.mak

PROFILER_BIN  = dlna-profiler
PROFILER_SRCS = dlna-profiler.c

SRCS = \
	$(PROFILER_SRCS) \

BINS = \
	$(PROFILER_BIN) \

CFLAGS  += -I../src
LDFLAGS += -L../src -ldlna

ifeq ($(BUILD_STATIC),yes)
  LDFLAGS += $(EXTRALIBS)
endif

all: banner $(PROFILER_BIN)

banner:
	@echo 
	@echo "#############################################"
	@echo "#         Building DLNA Utilities           #"
	@echo "#############################################"

$(PROFILER_BIN): $(PROFILER_SRCS)
	$(CC) $? $(OPTFLAGS) $(CFLAGS) $(LDFLAGS) -o $@

clean:
	-$(RM) -f $(BINS)

distclean: clean

.PHONY: clean distclean

dist-all:
	cp $(EXTRADIST) $(SRCS) Makefile $(DIST)

.PHONY: dist dist-all
