THREADUTIL_SRCS =  \
	threadutil/FreeList.c \
	threadutil/LinkedList.c \
	threadutil/ThreadPool.c \
	threadutil/TimerThread.c \

THREADUTIL_EXTRADIST = \
	threadutil/FreeList.h \
	threadutil/LinkedList.h \
	threadutil/ThreadPool.h \
	threadutil/TimerThread.h \
	threadutil/ithread.h \

THREADUTIL_OBJS = $(THREADUTIL_SRCS:.c=.o)
THREADUTIL_LOBJS = $(THREADUTIL_SRCS:.c=.lo)

all:

threadutil-dist-all:
	mkdir -p $(DIST)/threadutil
	cp $(THREADUTIL_EXTRADIST) $(THREADUTIL_SRCS) \
          threadutil.mak $(DIST)/threadutil
