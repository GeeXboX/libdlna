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
#include <fcntl.h>

#include "dlna_internals.h"
#include "upnp_internals.h"

#define UPNP_MAX_CONTENT_LENGTH 4096
#define VIRTUAL_DIR "/web"

static const upnp_service_t upnp_av_services[] = {
  { NULL, NULL, NULL }
};

static int
upnp_find_service_action (dlna_t *dlna,
                          upnp_service_t **service,
                          upnp_service_action_t **action,
                          struct Upnp_Action_Request *ar)
{
  int s, a;

  *service = NULL;
  *action = NULL;

  if (!ar || !ar->ActionName)
    return 0;

  /* parse all registered services */
  for (s = 0; upnp_av_services[s].id; s++)
  {
    /* find the resquested one */
    if (!strcmp (upnp_av_services[s].id, ar->ServiceID))
    {
      dlna_log (dlna, DLNA_MSG_INFO,
                "ActionRequest: using service %s\n", ar->ServiceID);
      *service = &upnp_av_services[s];
      /* parse all known actions */
      for (a = 0; upnp_av_services[s].actions[a].name; a++)
      {
        /* find the requested one */
        if (!strcmp (upnp_av_services[s].actions[a].name, ar->ActionName))
        {
          dlna_log (dlna, DLNA_MSG_INFO,
                    "ActionRequest: using action %s\n", ar->ActionName);
          *action = &upnp_av_services[s].actions[a];
          return 1;
        }
      }
      return 0;
    }
  }

  return 0;
}

static void
upnp_action_request_handler (dlna_t *dlna, struct Upnp_Action_Request *ar)
{
#if 0
  struct service_t *service;
  struct service_action_t *action;
#endif
  char val[256];
  uint32_t ip;

  if (!dlna || !ar)
    return;

  if (ar->ErrCode != UPNP_E_SUCCESS)
    return;

  /* ensure that message target is the specified device */
  if (strcmp (ar->DevUDN + 5, dlna->uuid))
    return;
  
  ip = ar->CtrlPtIPAddr.s_addr;
  ip = ntohl (ip);
  sprintf (val, "%d.%d.%d.%d",
           (ip >> 24) & 0xFF, (ip >> 16) & 0xFF, (ip >> 8) & 0xFF, ip & 0xFF);

  if (dlna->verbosity == DLNA_MSG_INFO)
  {
    DOMString str = ixmlPrintDocument (ar->ActionRequest);
    dlna_log (dlna, DLNA_MSG_INFO,
              "***************************************************\n");
    dlna_log (dlna, DLNA_MSG_INFO,
              "**             New Action Request                **\n");
    dlna_log (dlna, DLNA_MSG_INFO,
              "***************************************************\n");
    dlna_log (dlna, DLNA_MSG_INFO, "ServiceID: %s\n", ar->ServiceID);
    dlna_log (dlna, DLNA_MSG_INFO, "ActionName: %s\n", ar->ActionName);
    dlna_log (dlna, DLNA_MSG_INFO, "CtrlPtIP: %s\n", val);
    dlna_log (dlna, DLNA_MSG_INFO, "Action Request:\n%s\n", str);
    ixmlFreeDOMString (str);
  }

#if 0
  /* not yet implemented */
  if (find_service_action (ar, &service, &action))
  {
    struct action_event_t event;

    event.request = ar;
    event.status  = true;
    event.service = service;

    if (action->function (&event) && event.status)
      ar->ErrCode = UPNP_E_SUCCESS;

    if (dlna->verbosity == DLNA_MSG_INFO)
    {
      DOMString str = ixmlPrintDocument (ar->ActionResult);
      dlna_log (dlna, DLNA_MSG_INFO, "Action Result:\n%s", str);
      dlna_log (dlna, DLNA_MSG_INFO,
                "***************************************************\n");
      dlna_log (dlna, DLNA_MSG_INFO, "\n");
      ixmlFreeDOMString (str);
    }
      
    return;
  }

  if (service) /* Invalid Action name */
    strcpy (ar->ErrStr, "Unknown Service Action");
  else /* Invalid Service name */
    strcpy (ar->ErrStr, "Unknown Service ID");
#endif
  
  ar->ActionResult = NULL;
  ar->ErrCode = UPNP_SOAP_E_INVALID_ACTION;
}

static int
device_callback_event_handler (Upnp_EventType type,
                               void *event,
                               void *cookie)
{
  switch (type)
  {
  case UPNP_CONTROL_ACTION_REQUEST:
    upnp_action_request_handler ((dlna_t *) cookie,
                                 (struct Upnp_Action_Request *) event);
    break;
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

  dlna_log (dlna, DLNA_MSG_INFO, "Initializing UPnP A/V subsystem ...\n");

  ip = get_iface_address (dlna->interface);
  if (!ip)
    goto upnp_init_err;
  
  res = UpnpInit (ip, dlna->port);
  if (res != UPNP_E_SUCCESS)
  {
    dlna_log (dlna, DLNA_MSG_CRITICAL,
              "Cannot initialize UPnP A/V subsystem\n");
    goto upnp_init_err;
  }

  if (UpnpSetMaxContentLength (UPNP_MAX_CONTENT_LENGTH) != UPNP_E_SUCCESS)
    dlna_log (dlna, DLNA_MSG_ERROR, "Could not set UPnP max content length\n");

  dlna->port = UpnpGetServerPort ();
  dlna_log (dlna, DLNA_MSG_INFO, "UPnP MediaServer listening on %s:%d\n",
            UpnpGetServerIpAddress (), dlna->port);

  UpnpEnableWebserver (TRUE);

  res = UpnpSetVirtualDirCallbacks (&virtual_dir_callbacks);
  if (res != UPNP_E_SUCCESS)
  {
    dlna_log (dlna, DLNA_MSG_CRITICAL,
              "Cannot set virtual directory callbacks\n");
    goto upnp_init_err;
  }
  
  res = UpnpAddVirtualDir (VIRTUAL_DIR);
  if (res != UPNP_E_SUCCESS)
  {
    dlna_log (dlna, DLNA_MSG_CRITICAL,
              "Cannot add virtual directory for web server\n");
    goto upnp_init_err;
  }

  res = UpnpRegisterRootDevice2 (UPNPREG_BUF_DESC, description, 0, 1,
                                 device_callback_event_handler,
                                 dlna, &(dlna->dev));
  if (res != UPNP_E_SUCCESS)
  {
    dlna_log (dlna, DLNA_MSG_CRITICAL, "Cannot register UPnP A/V device\n");
    goto upnp_init_err;
  }

  res = UpnpUnRegisterRootDevice (dlna->dev);
  if (res != UPNP_E_SUCCESS)
  {
    dlna_log (dlna, DLNA_MSG_CRITICAL, "Cannot unregister UPnP device\n");
    goto upnp_init_err;
  }

  res = UpnpRegisterRootDevice2 (UPNPREG_BUF_DESC, description, 0, 1,
                                 device_callback_event_handler,
                                 NULL, &(dlna->dev));
  if (res != UPNP_E_SUCCESS)
  {
    dlna_log (dlna, DLNA_MSG_CRITICAL, "Cannot register UPnP device\n");
    goto upnp_init_err;
  }

  dlna_log (dlna, DLNA_MSG_INFO,
            "Sending UPnP advertisement for device ...\n");
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

  dlna_log (dlna, DLNA_MSG_INFO, "Stopping UPnP A/V Service ...\n");
  UpnpUnRegisterRootDevice (dlna->dev);
  UpnpFinish ();

  return UPNP_E_SUCCESS;
}
