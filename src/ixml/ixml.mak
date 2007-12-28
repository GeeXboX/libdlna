IXML_SRCS =  \
	ixml/ixml.c \
	ixml/node.c \
	ixml/ixmlparser.c \
	ixml/ixmlmembuf.c \
	ixml/nodeList.c \
	ixml/element.c \
	ixml/attr.c \
	ixml/document.c \
	ixml/namedNodeMap.c \

IXML_EXTRADIST = \
	ixml/ixmlmembuf.h \
	ixml/ixmlparser.h \
	ixml/ixml.h \

IXML_OBJS = $(IXML_SRCS:.c=.o)
IXML_LOBJS = $(IXML_SRCS:.c=.lo)

all:

ixml-dist-all:
	mkdir -p $(DIST)/ixml
	cp $(IXML_EXTRADIST) $(IXML_SRCS) ixml.mak $(DIST)/ixml
