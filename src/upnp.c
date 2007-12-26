/*
 * libdlna: reference DLNA standards implementation.
 * Copyright (C) 2007 Benjamin Zores <ben@geexbox.org>
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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
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

#include <upnp/upnp.h>
#include <upnp/upnptools.h>

#include "dlna_internals.h"

#define UPNP_MAX_CONTENT_LENGTH 4096
#define VIRTUAL_DIR "/web"

void
dlna_set_device_friendly_name (dlna_t *dlna, char *str)
{
  if (!dlna || !str)
    return;

  if (dlna->friendly_name)
    free (dlna->friendly_name);
  dlna->friendly_name = strdup (str);
}

void
dlna_set_device_manufacturer (dlna_t *dlna, char *str)
{
  if (!dlna || !str)
    return;

  if (dlna->manufacturer)
    free (dlna->manufacturer);
  dlna->manufacturer = strdup (str);
}

void
dlna_set_device_manufacturer_url (dlna_t *dlna, char *str)
{
  if (!dlna || !str)
    return;

  if (dlna->manufacturer_url)
    free (dlna->manufacturer_url);
  dlna->manufacturer_url = strdup (str);
}

void
dlna_set_device_model_description (dlna_t *dlna, char *str)
{
  if (!dlna || !str)
    return;

  if (dlna->model_description)
    free (dlna->model_description);
  dlna->model_description = strdup (str);
}

void
dlna_set_device_model_name (dlna_t *dlna, char *str)
{
  if (!dlna || !str)
    return;

  if (dlna->model_name)
    free (dlna->model_name);
  dlna->model_name = strdup (str);
}

void
dlna_set_device_model_number (dlna_t *dlna, char *str)
{
  if (!dlna || !str)
    return;

  if (dlna->model_number)
    free (dlna->model_number);
  dlna->model_number = strdup (str);
}

void
dlna_set_device_model_url (dlna_t *dlna, char *str)
{
  if (!dlna || !str)
    return;

  if (dlna->model_url)
    free (dlna->model_url);
  dlna->model_url = strdup (str);
}


void
dlna_set_device_serial_number (dlna_t *dlna, char *str)
{
  if (!dlna || !str)
    return;

  if (dlna->serial_number)
    free (dlna->serial_number);
  dlna->serial_number = strdup (str);
}

void
dlna_set_device_uuid (dlna_t *dlna, char *str)
{
  if (!dlna || !str)
    return;

  if (dlna->uuid)
    free (dlna->uuid);
  dlna->uuid = strdup (str);
}

static int
device_callback_event_handler (Upnp_EventType type,
                               void *event dlna_unused,
                               void *cookie dlna_unused)
{
  switch (type)
    {
    case UPNP_CONTROL_ACTION_REQUEST:
    case UPNP_CONTROL_ACTION_COMPLETE:
    case UPNP_EVENT_SUBSCRIPTION_REQUEST:
    case UPNP_CONTROL_GET_VAR_REQUEST:
      break;
    default:
      break;
    }

  return 0;
}

static char *
get_iface_address (char *interface)
{
  int sock;
  uint32_t ip;
  struct ifreq ifr;
  char *val;

  if (!interface)
    return NULL;

  /* determine UDN according to MAC address */
  sock = socket (AF_INET, SOCK_STREAM, 0);
  if (sock < 0)
  {
    perror ("socket");
    return NULL;
  }

  strcpy (ifr.ifr_name, interface);
  ifr.ifr_addr.sa_family = AF_INET;

  if (ioctl (sock, SIOCGIFADDR, &ifr) < 0)
  {
    perror ("ioctl");
    close (sock);
    return NULL;
  }

  val = malloc (16 * sizeof (char));
  ip = ((struct sockaddr_in *) &ifr.ifr_addr)->sin_addr.s_addr;
  ip = ntohl (ip);
  sprintf (val, "%d.%d.%d.%d",
           (ip >> 24) & 0xFF, (ip >> 16) & 0xFF, (ip >> 8) & 0xFF, ip & 0xFF);

  close (sock);
  return val;
}

int
upnp_init (dlna_t *dlna, dlna_device_type_t type)
{
  char *description = NULL;
  char *ip = NULL;
  int res;

  if (!dlna)
    return -1;

  if (type == DLNA_DEVICE_UNKNOWN)
    return -1;

  switch (type)
  {
  case DLNA_DEVICE_DMS:
    description =
      dlna_dms_description_get (dlna->friendly_name,
                                dlna->manufacturer,
                                dlna->manufacturer_url,
                                dlna->model_description,
                                dlna->model_name,
                                dlna->model_number,
                                dlna->model_url,
                                dlna->serial_number,
                                dlna->uuid,
                                "/web/presentation.html",
                                "/web/cms.xml",
                                "/web/cms_control",
                                "/web/cms_event",
                                "/web/cds.xml",
                                "/web/cds_control",
                                "/web/cds_event");
  case DLNA_DEVICE_DMP:
  default:
    break;
  }
  
  if (!description)
    goto upnp_init_err;

  if (dlna->verbosity)
    fprintf (stderr, "Initializing UPnP A/V subsystem ...\n");

  ip = get_iface_address (dlna->interface);
  if (!ip)
    goto upnp_init_err;
  
#if 0 /* not yet implemented */
  res = UpnpInit (dlna->ip, ut->port);
  if (res != UPNP_E_SUCCESS)
  {
    if (dlna->verbosity)
      fprintf (stderr, "Cannot initialize UPnP A/V subsystem\n");
    goto upnp_init_err;
  }
#endif

  if (UpnpSetMaxContentLength (UPNP_MAX_CONTENT_LENGTH) != UPNP_E_SUCCESS)
  {
    if (dlna->verbosity)
      fprintf (stderr, "Could not set UPnP max content length\n");
  }

  dlna->port = UpnpGetServerPort ();
  if (dlna->verbosity)
    fprintf (stderr, "UPnP MediaServer listening on %s:%d\n",
             UpnpGetServerIpAddress (), dlna->port);

  UpnpEnableWebserver (TRUE);

#if 0 /* not yet implemented */
  res = UpnpSetVirtualDirCallbacks (&virtual_dir_callbacks);
  if (res != UPNP_E_SUCCESS)
  {
    if (dlna->verbosity)
      fprintf (stderr, "Cannot set virtual directory callbacks\n");
    goto upnp_init_err;
  }
#endif
  
  res = UpnpAddVirtualDir (VIRTUAL_DIR);
  if (res != UPNP_E_SUCCESS)
  {
    if (dlna->verbosity)
      fprintf (stderr, "Cannot add virtual directory for web server\n");
    goto upnp_init_err;
  }

  res = UpnpRegisterRootDevice2 (UPNPREG_BUF_DESC, description, 0, 1,
                                 device_callback_event_handler,
                                 NULL, &(dlna->dev));
  if (res != UPNP_E_SUCCESS)
  {
    if (dlna->verbosity)
      fprintf (stderr, "Cannot register UPnP A/V device\n");
    goto upnp_init_err;
  }

  res = UpnpUnRegisterRootDevice (dlna->dev);
  if (res != UPNP_E_SUCCESS)
  {
    if (dlna->verbosity)
      fprintf (stderr, "Cannot unregister UPnP device\n");
    goto upnp_init_err;
  }

  res = UpnpRegisterRootDevice2 (UPNPREG_BUF_DESC, description, 0, 1,
                                 device_callback_event_handler,
                                 NULL, &(dlna->dev));
  if (res != UPNP_E_SUCCESS)
  {
    if (dlna->verbosity)
      fprintf (stderr, "Cannot register UPnP device\n");
    goto upnp_init_err;
  }

  if (dlna->verbosity)
    fprintf (stderr, "Sending UPnP advertisement for device ...\n");
  UpnpSendAdvertisement (dlna->dev, 1800);

  free (ip);
  free (description);
  return 0;

 upnp_init_err:
  if (ip)
    free (ip);
  if (description)
    free (description);
  return -1;
}

int
upnp_uninit (dlna_t *dlna)
{
  if (!dlna)
    return -1;

  if (dlna->verbosity)
    fprintf (stderr, "Stopping UPnP A/V Service ...\n");
  UpnpUnRegisterRootDevice (dlna->dev);
  UpnpFinish ();

  return UPNP_E_SUCCESS;
}
