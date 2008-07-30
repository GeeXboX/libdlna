/*
 * libdlna: reference DLNA standards implementation.
 * Copyright (C) 2007-2008 Benjamin Zores <ben@geexbox.org>
 *
 * This file is part of libdlna.
 *
 * libdlna is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * libdlna is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with libdlna; if not, write to the Free Software
 * Foundation, Inc, 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#if (defined(BSD) || defined(__FreeBSD__) || defined(__APPLE__))
#include <sys/socket.h>
#include <sys/sysctl.h>
#include <net/if_dl.h>
#endif

#if (defined(__APPLE__))
#include <net/route.h>
#endif

#include <net/if.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdbool.h>
#include <fcntl.h>

#ifdef HAVE_IFADDRS_H
#include <ifaddrs.h>
#endif

#include "dlna_internals.h"
#include "profiles.h"
#include "containers.h"

dlna_t *
dlna_init (void)
{
  dlna_t *dlna;

  dlna = malloc (sizeof (dlna_t));
  dlna->inited = 1;
  dlna->verbosity = DLNA_MSG_ERROR;
  dlna->mode = DLNA_CAPABILITY_DLNA;
  dlna->check_extensions = 1;
  dlna->first_profile = NULL;
  dlna->flags = 0;

  /* Internal HTTP Server */
  dlna->http_callback = NULL;
  
  dlna->vfs_root = NULL;
  dlna->vfs_items = 0;
  dlna_vfs_add_container (dlna, "root", 0, 0);
  
  dlna->interface = strdup ("lo"); /* bind to loopback as a default */
  dlna->port = 0;
  
  /* UPnP Properties */
  dlna->friendly_name = strdup ("libdlna");
  dlna->manufacturer = strdup ("Benjamin Zores");
  dlna->manufacturer_url = strdup ("http://libdlna.geexbox.org/");
  dlna->model_description = strdup ("libdlna device");
  dlna->model_name = strdup ("libdlna");
  dlna->model_number = strdup ("libdlna-001");
  dlna->model_url = strdup ("http://libdlna.geexbox.org/");
  dlna->serial_number = strdup ("libdlna-001");
  dlna->uuid = strdup ("01:23:45:67:89");
  dlna->presentation_url = strdup ("presentation.html");
  
  /* register all FFMPEG demuxers */
  av_register_all ();

  dlna_log (dlna, DLNA_MSG_INFO, "DLNA: init\n");
  
  return dlna;
}

void
dlna_uninit (dlna_t *dlna)
{
  if (!dlna)
    return;

  dlna->inited = 0;
  dlna_log (dlna, DLNA_MSG_INFO, "DLNA: uninit\n");
  dlna->first_profile = NULL;
  vfs_item_free (dlna, dlna->vfs_root);
  free (dlna->interface);

  /* Internal HTTP Server */
  if (dlna->http_callback)
    free (dlna->http_callback);
  
  /* UPnP Properties */
  free (dlna->friendly_name);
  free (dlna->manufacturer);
  free (dlna->manufacturer_url);
  free (dlna->model_description);
  free (dlna->model_name);
  free (dlna->model_number);
  free (dlna->model_url);
  free (dlna->serial_number);
  free (dlna->uuid);
  free (dlna->presentation_url);

  free (dlna);
}

void
dlna_log (dlna_t *dlna, dlna_verbosity_level_t level, const char *format, ...)
{
  va_list va;

  if (!dlna || !format)
    return;

  /* do we really want loging ? */
  if (dlna->verbosity == DLNA_MSG_NONE)
    return;
  
  if (level < dlna->verbosity)
    return;

  va_start (va, format);
  fprintf (stderr, "[libdlna] ");
  vfprintf (stderr, format, va);
  va_end (va);
}

void
dlna_set_verbosity (dlna_t *dlna, dlna_verbosity_level_t level)
{
  if (!dlna)
    return;

  dlna->verbosity = level;
}

void
dlna_set_capability_mode (dlna_t *dlna, dlna_capability_mode_t mode)
{
  if (!dlna)
    return;

  dlna->mode = mode;

  if (dlna->mode != DLNA_CAPABILITY_DLNA)
    dlna->check_extensions = 1;
}

void
dlna_set_org_flags (dlna_t *dlna, dlna_org_flags_t flags)
{
  if (!dlna)
    return;

  dlna->flags = flags;
}

void
dlna_set_extension_check (dlna_t *dlna, int level)
{
  if (!dlna)
    return;

  if (dlna->mode != DLNA_CAPABILITY_DLNA)
    return;
  
  dlna->check_extensions = level;
}

static int
has_network_interface (char *interface)
{
#ifdef HAVE_IFADDRS_H
  struct ifaddrs *itflist, *itf;

  if (!interface)
    return 0;

  if (getifaddrs (&itflist) < 0)
  {
    perror ("getifaddrs");
    return 0;
  }

  itf = itflist;
  while (itf)
  {
    if ((itf->ifa_flags & IFF_UP)
        && !strncmp (itf->ifa_name, interface, IFNAMSIZ))
    {
      freeifaddrs (itflist);
      return 1;
    }
    itf = itf->ifa_next;
  }

  freeifaddrs (itflist);
#else  
  int sock, i, n;
  struct ifconf ifc;
  struct ifreq ifr;
  char buff[8192];

  if (!interface)
    return 0;

  /* determine UDN according to MAC address */
  sock = socket (AF_INET, SOCK_STREAM, 0);
  if (sock < 0)
  {
    perror ("socket");
    return 0;
  }

  /* get list of available interfaces */
  ifc.ifc_len = sizeof (buff);
  ifc.ifc_buf = buff;

  if (ioctl (sock, SIOCGIFCONF, &ifc) < 0)
  {
    perror ("ioctl");
    close (sock);
    return 0;
  }

  n = ifc.ifc_len / sizeof (struct ifreq);
  for (i = n - 1 ; i >= 0 ; i--)
  {
    ifr = ifc.ifc_req[i];

    if (strncmp (ifr.ifr_name, interface, IFNAMSIZ))
      continue;

    if (ioctl (sock, SIOCGIFFLAGS, &ifr) < 0)
    {
      perror ("ioctl");
      close (sock);
      return 0;
    }

    if (!(ifr.ifr_flags & IFF_UP))
    {
      close (sock);
      return 0;
    }

    /* found right interface */
    close (sock);
    return 1;
  }
  close (sock);
#endif

  return 0;
}

void
dlna_set_interface (dlna_t *dlna, char *itf)
{
  if (!dlna || !itf)
    return;

  /* check for valid network interface */
  if (!has_network_interface (itf))
    return;
  
  if (dlna->interface)
    free (dlna->interface);
  dlna->interface = strdup (itf);
}

void
dlna_set_port (dlna_t *dlna, int port)
{
  if (!dlna)
    return;

  /* check for valid port number */
  if (port <= 0 || port > 65535)
    return;
  
  dlna->port = port;
}

void
dlna_device_set_friendly_name (dlna_t *dlna, char *str)
{
  if (!dlna || !str)
    return;

  if (dlna->friendly_name)
    free (dlna->friendly_name);
  dlna->friendly_name = strdup (str);
}

void
dlna_device_set_manufacturer (dlna_t *dlna, char *str)
{
  if (!dlna || !str)
    return;

  if (dlna->manufacturer)
    free (dlna->manufacturer);
  dlna->manufacturer = strdup (str);
}

void
dlna_device_set_manufacturer_url (dlna_t *dlna, char *str)
{
  if (!dlna || !str)
    return;

  if (dlna->manufacturer_url)
    free (dlna->manufacturer_url);
  dlna->manufacturer_url = strdup (str);
}

void
dlna_device_set_model_description (dlna_t *dlna, char *str)
{
  if (!dlna || !str)
    return;

  if (dlna->model_description)
    free (dlna->model_description);
  dlna->model_description = strdup (str);
}

void
dlna_device_set_model_name (dlna_t *dlna, char *str)
{
  if (!dlna || !str)
    return;

  if (dlna->model_name)
    free (dlna->model_name);
  dlna->model_name = strdup (str);
}

void
dlna_device_set_model_number (dlna_t *dlna, char *str)
{
  if (!dlna || !str)
    return;

  if (dlna->model_number)
    free (dlna->model_number);
  dlna->model_number = strdup (str);
}

void
dlna_device_set_model_url (dlna_t *dlna, char *str)
{
  if (!dlna || !str)
    return;

  if (dlna->model_url)
    free (dlna->model_url);
  dlna->model_url = strdup (str);
}


void
dlna_device_set_serial_number (dlna_t *dlna, char *str)
{
  if (!dlna || !str)
    return;

  if (dlna->serial_number)
    free (dlna->serial_number);
  dlna->serial_number = strdup (str);
}

void
dlna_device_set_uuid (dlna_t *dlna, char *str)
{
  if (!dlna || !str)
    return;

  if (dlna->uuid)
    free (dlna->uuid);
  dlna->uuid = strdup (str);
}

void
dlna_device_set_presentation_url (dlna_t *dlna, char *str)
{
  if (!dlna || !str)
    return;

  if (dlna->presentation_url)
    free (dlna->presentation_url);
  dlna->presentation_url = strdup (str);
}

void
dlna_set_http_callback (dlna_t *dlna, dlna_http_callback_t *cb)
{
  if (!dlna)
    return;

  dlna->http_callback = cb;
}

char *
dlna_write_protocol_info (dlna_protocol_info_type_t type,
                          dlna_org_play_speed_t speed,
                          dlna_org_conversion_t ci,
                          dlna_org_operation_t op,
                          dlna_org_flags_t flags,
                          dlna_profile_t *p)
{
  char protocol[512];
  char dlna_info[448];
 
  if (type == DLNA_PROTOCOL_INFO_TYPE_HTTP)
    sprintf (protocol, "http-get:*:");

  strcat (protocol, p->mime);
  strcat (protocol, ":");

  if (p->id)
  {
    sprintf (dlna_info, "%s=%d;%s=%d;%s=%.2x;%s=%s;%s=%.8x%.24x",
             "DLNA.ORG_PS", speed, "DLNA.ORG_CI", ci,
             "DLNA.ORG_OP", op, "DLNA.ORG_PN", p->id,
             "DLNA.ORG_FLAGS", flags, 0);
    strcat (protocol, dlna_info);
  }
  else
    strcat (protocol, "*");

  return strdup (protocol);
}
