include ../config.mak

LIBNAME = libdlna.so

SRCS =  profiles.c \
	image_jpeg.c \
	image_png.c \
	audio_ac3.c \
	audio_amr.c \
	audio_atrac3.c \
	audio_lpcm.c \
	audio_mp3.c \
	audio_mpeg4.c \

CFLAGS += -fPIC

OBJS = $(SRCS:.c=.o)

.SUFFIXES: .c .o

all: depend $(LIBNAME)

.c.o:
	$(CC) -c $(CFLAGS) $(OPTFLAGS) -o $@ $<

$(LIBNAME): $(OBJS)
	$(CC) -shared $(OBJS) $(LDFLAGS) $(EXTRALIBS) -o $(LIBNAME)

clean:
	rm -f *.o *.a *.so
	rm -f .depend

install: $(LIBNAME)
	$(INSTALL) -c $(LIBNAME) $(libdir)
	$(INSTALL) -c -m 644 dlna.h $(includedir)

depend:
	$(CC) -MM $(CFLAGS) $(SRCS) 1>.depend

.phony: clean depend

#
# include dependency files if they exist
#
ifneq ($(wildcard .depend),)
include .depend
endif
