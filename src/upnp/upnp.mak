# SSDP
UPNP_SRCS +=  \
	upnp/ssdp_device.c \
	upnp/ssdp_ctrlpt.c \
	upnp/ssdp_server.c \

# SOAP
UPNP_SRCS += \
	upnp/soap_device.c \
	upnp/soap_ctrlpt.c \
	upnp/soap_common.c \

# GENLIB
UPNP_SRCS += \
	upnp/miniserver.c \
	upnp/service_table.c \
	upnp/membuffer.c \
	upnp/strintmap.c \
	upnp/upnp_timeout.c \
	upnp/util.c \
	upnp/client_table.c \
	upnp/sock.c \
	upnp/httpparser.c \
	upnp/httpreadwrite.c \
	upnp/statcodes.c \
	upnp/webserver.c \
	upnp/parsetools.c \
	upnp/uri.c \

# GENA
UPNP_SRCS += \
	upnp/gena_device.c \
	upnp/gena_ctrlpt.c \
	upnp/gena_callback2.c \

# API
UPNP_SRCS += upnp/upnpapi.c
UPNP_SRCS += upnp/upnptools.c
ifeq ($(DEBUG),yes)
UPNP_SRCS += upnp/upnpdebug.c 
endif

# UUID
UPNP_SRCS += \
	upnp/md5.c \
	upnp/sysdep.c \
	upnp/uuid.c \

# URLconfig
UPNP_SRCS += upnp/urlconfig.c


UPNP_EXTRADIST = \
	upnp/client_table.h \
	upnp/config.h \
	upnp/gena_ctrlpt.h \
	upnp/gena_device.h \
	upnp/gena.h \
	upnp/global.h \
	upnp/gmtdate.h \
	upnp/http_client.h \
	upnp/httpparser.h \
	upnp/httpreadwrite.h \
	upnp/md5.h \
	upnp/membuffer.h \
	upnp/miniserver.h \
	upnp/netall.h \
	upnp/parsetools.h \
	upnp/server.h \
	upnp/service_table.h \
	upnp/soaplib.h \
	upnp/sock.h \
	upnp/ssdplib.h \
	upnp/statcodes.h \
	upnp/statuscodes.h \
	upnp/uri.h \
	upnp/strintmap.h \
	upnp/sysdep.h \
	upnp/unixutil.h \
	upnp/upnpapi.h \
	upnp/upnpclosesocket.h \
	upnp/webserver.h \
	upnp/upnpdebug.h \
	upnp/upnp.h \
	upnp/upnp_timeout.h \
	upnp/upnptools.h \
	upnp/urlconfig.h \
	upnp/utilall.h \
	upnp/util.h \
	upnp/uuid.h \

UPNP_OBJS = $(UPNP_SRCS:.c=.o)
UPNP_LOBJS = $(UPNP_SRCS:.c=.lo)

all:

upnp-dist-all:
	mkdir -p $(DIST)/upnp
	cp $(UPNP_EXTRADIST) $(UPNP_SRCS) upnp.mak $(DIST)/upnp
