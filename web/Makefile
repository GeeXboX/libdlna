ifeq (,$(wildcard ../config.mak))
$(error "../config.mak is not present, run configure !")
endif
include ../config.mak

EXTRADIST = index.html \

all:

dist-all:
	cp -pR $(EXTRADIST) $(SRCS) Makefile $(DIST)

.PHONY: dist-all
