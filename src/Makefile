ifeq (,$(wildcard ../config.mak))
$(error "../config.mak is not present, run configure !")
endif
include ../config.mak

LIBNAMEBASE = libdlna.so
LIBNAMEEXT = $(LIBNAMEBASE).$(VERSION)
LIBNAME = $(LIBNAMEBASE).$(shell echo $(VERSION) | cut -f1 -d.)

SRCS =  profiles.c \
	containers.c \
	image_jpeg.c \
	image_png.c \
	audio_aac.c \
	audio_ac3.c \
	audio_amr.c \
	audio_atrac3.c \
	audio_g726.c \
	audio_lpcm.c \
	audio_mp2.c \
	audio_mp3.c \
	audio_wma.c \
	av_mpeg1.c \
	av_mpeg2.c \
	av_mpeg4_part2.c \
	av_mpeg4_part10.c \
	av_wmv9.c \

EXTRADIST = dlna.h \
	containers.h \
	profiles.h \

CFLAGS += -fPIC

ifeq ($(DEBUG),yes)
CFLAGS += -DHAVE_DEBUG
endif

OBJS = $(SRCS:.c=.o)

.SUFFIXES: .c .o

all: depend $(LIBNAMEEXT)

.c.o:
	$(CC) -c $(CFLAGS) $(OPTFLAGS) -o $@ $<

$(LIBNAMEEXT): $(OBJS)
	$(CC) -shared -Wl,-soname,$@ $(OBJS) $(LDFLAGS) $(EXTRALIBS) -o $@
	$(LN) -sf $@ $(LIBNAME)
	$(LN) -sf $@ $(LIBNAMEBASE)

clean:
	-$(RM) -f *.o *.a *.so*
	-$(RM) -f .depend

install: $(LIBNAME)
	$(INSTALL) -d $(libdir)
	$(INSTALL) $(LIBNAMEEXT) $(libdir)
	$(LN) -sf $(LIBNAMEEXT) $(libdir)/$(LIBNAME)
	$(LN) -sf $(LIBNAMEEXT) $(libdir)/$(LIBNAMEBASE)
	$(INSTALL) -d $(includedir)
	$(INSTALL) -m 644 dlna.h $(includedir)

depend:
	$(CC) -MM $(CFLAGS) $(SRCS) 1>.depend

.PHONY: clean depend

dist-all:
	cp $(EXTRADIST) $(SRCS) Makefile $(DIST)

.PHONY: dist-all

#
# include dependency files if they exist
#
ifneq ($(wildcard .depend),)
include .depend
endif
