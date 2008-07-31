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

#include "upnp_internals.h"

static int
upnp_find_service_action (dlna_t *dlna,
                          upnp_service_t **service,
                          upnp_service_action_t **action,
                          struct dlna_Action_Request *ar)
{
  int a;
  upnp_service_t *srv;
  
  *service = NULL;
  *action = NULL;

  if (!ar || !ar->ActionName)
    return DLNA_ST_ERROR;

  dlna_log (dlna, DLNA_MSG_INFO,
            "ActionRequest: using service %s\n", ar->ServiceID);
  
  /* find the resquested service in all registered ones */
  srv = dlna_service_find (dlna, ar->ServiceID);
  if (!srv)
    return DLNA_ST_ERROR;
  
  /* parse all known actions */
  for (a = 0; srv->actions[a].name; a++)
  {
    /* find the requested one */
    if (!strcmp (srv->actions[a].name, ar->ActionName))
    {
      dlna_log (dlna, DLNA_MSG_INFO,
                "ActionRequest: using action %s\n", ar->ActionName);
      *service = srv;
      *action = &srv->actions[a];
      return DLNA_ST_OK;
    }
  }

  return DLNA_ST_ERROR;
}

static void
upnp_action_request_handler (dlna_t *dlna, struct dlna_Action_Request *ar)
{
  upnp_service_t *service;
  upnp_service_action_t *action;
  char val[256];
  uint32_t ip;

  if (!dlna || !ar)
    return;

  if (ar->ErrCode != DLNA_E_SUCCESS)
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

  if (upnp_find_service_action (dlna, &service, &action, ar) == DLNA_ST_OK)
  {
    upnp_action_event_t event;

    event.ar      = ar;
    event.status  = 1;
    event.service = service;

    if (action->cb (dlna, &event) && event.status)
      ar->ErrCode = DLNA_E_SUCCESS;

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
  
  ar->ActionResult = NULL;
  ar->ErrCode = DLNA_SOAP_E_INVALID_ACTION;
}

static int
device_callback_event_handler (dlna_EventType type,
                               void *event,
                               void *cookie)
{
  switch (type)
  {
  case DLNA_CONTROL_ACTION_REQUEST:
    upnp_action_request_handler ((dlna_t *) cookie,
                                 (struct dlna_Action_Request *) event);
    break;
  case DLNA_CONTROL_ACTION_COMPLETE:
  case DLNA_EVENT_SUBSCRIPTION_REQUEST:
  case DLNA_CONTROL_GET_VAR_REQUEST:
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
    return DLNA_ST_ERROR;

  if (type == DLNA_DEVICE_UNKNOWN)
    return DLNA_ST_ERROR;

  switch (type)
  {
  case DLNA_DEVICE_DMS:
  {
    description = dlna_dms_description_get (dlna);
    break;
  }
  
  case DLNA_DEVICE_DMP:
  default:
    break;
  }
  
  if (!description)
    goto upnp_init_err;

  dlna_log (dlna, DLNA_MSG_INFO, "Initializing UPnP subsystem ...\n");

  ip = get_iface_address (dlna->interface);
  if (!ip)
    goto upnp_init_err;
  
  res = dlnaInit (ip, dlna->port);
  if (res != DLNA_E_SUCCESS)
  {
    dlna_log (dlna, DLNA_MSG_CRITICAL,
              "Cannot initialize UPnP subsystem\n");
    goto upnp_init_err;
  }

  if (dlnaSetMaxContentLength (DLNA_MAX_CONTENT_LENGTH) != DLNA_E_SUCCESS)
    dlna_log (dlna, DLNA_MSG_ERROR, "Could not set UPnP max content length\n");

  dlna->port = dlnaGetServerPort ();
  dlna_log (dlna, DLNA_MSG_INFO, "UPnP device listening on %s:%d\n",
            dlnaGetServerIpAddress (), dlna->port);

  dlnaEnableWebserver (TRUE);

  res = dlnaSetVirtualDirCallbacks (&virtual_dir_callbacks, dlna);
  if (res != DLNA_E_SUCCESS)
  {
    dlna_log (dlna, DLNA_MSG_CRITICAL,
              "Cannot set virtual directory callbacks\n");
    goto upnp_init_err;
  }
  
  res = dlnaAddVirtualDir (VIRTUAL_DIR);
  if (res != DLNA_E_SUCCESS)
  {
    dlna_log (dlna, DLNA_MSG_CRITICAL,
              "Cannot add virtual directory for web server\n");
    goto upnp_init_err;
  }

  res = dlnaAddVirtualDir (SERVICES_VIRTUAL_DIR);
  if (res != DLNA_E_SUCCESS)
  {
    dlna_log (dlna, DLNA_MSG_CRITICAL,
              "Cannot add virtual directory for services\n");
    goto upnp_init_err;
  }

  res = dlnaRegisterRootDevice2 (DLNAREG_BUF_DESC, description, 0, 1,
                                 device_callback_event_handler,
                                 dlna, &(dlna->dev));
  if (res != DLNA_E_SUCCESS)
  {
    dlna_log (dlna, DLNA_MSG_CRITICAL, "Cannot register UPnP device\n");
    goto upnp_init_err;
  }

  res = dlnaUnRegisterRootDevice (dlna->dev);
  if (res != DLNA_E_SUCCESS)
  {
    dlna_log (dlna, DLNA_MSG_CRITICAL, "Cannot unregister UPnP device\n");
    goto upnp_init_err;
  }

  res = dlnaRegisterRootDevice2 (DLNAREG_BUF_DESC, description, 0, 1,
                                 device_callback_event_handler,
                                 dlna, &(dlna->dev));
  if (res != DLNA_E_SUCCESS)
  {
    dlna_log (dlna, DLNA_MSG_CRITICAL, "Cannot register UPnP device\n");
    goto upnp_init_err;
  }

  dlna_log (dlna, DLNA_MSG_INFO,
            "Sending UPnP advertisement for device ...\n");
  dlnaSendAdvertisement (dlna->dev, 1800);

  free (ip);
  free (description);
  return DLNA_ST_OK;

 upnp_init_err:
  if (ip)
    free (ip);
  if (description)
    free (description);
  return DLNA_ST_ERROR;
}

int
upnp_uninit (dlna_t *dlna)
{
  if (!dlna)
    return DLNA_ST_ERROR;

  dlna_log (dlna, DLNA_MSG_INFO, "Stopping UPnP A/V Service ...\n");
  dlnaUnRegisterRootDevice (dlna->dev);
  dlnaFinish ();

  return DLNA_ST_OK;
}

int
upnp_add_response (upnp_action_event_t *ev, char *key, const char *value)
{
  char *val;
  int res;

  if (!ev || !ev->status || !key || !value)
    return 0;

  val = strdup (value);
  res = dlnaAddToActionResponse (&ev->ar->ActionResult,
                                 ev->ar->ActionName,
                                 ev->service->type, key, val);

  if (res != DLNA_E_SUCCESS)
  {
    free (val);
    return 0;
  }

  free (val);
  return 1;
}

char *
upnp_get_string (struct dlna_Action_Request *ar, const char *key)
{
  IXML_Node *node = NULL;

  if (!ar || !ar->ActionRequest || !key)
    return NULL;

  node = (IXML_Node *) ar->ActionRequest;
  if (!node)
    return NULL;

  node = ixmlNode_getFirstChild (node);
  if (!node)
    return NULL;

  node = ixmlNode_getFirstChild (node);
  for (; node; node = ixmlNode_getNextSibling (node))
    if (ixmlNode_getNodeName (node) &&
        !strcmp (ixmlNode_getNodeName (node), key))
    {
      node = ixmlNode_getFirstChild (node);
      if (!node)
        return strdup ("");
      return ixmlNode_getNodeValue (node) ?
        strdup (ixmlNode_getNodeValue (node)) : strdup ("");
    }

  return NULL;
}

int
upnp_get_ui4 (struct dlna_Action_Request *ar, const char *key)
{
  char *value;
  int val;

  if (!ar || !key)
    return 0;

  value = upnp_get_string (ar, key);
  if (!value && !strcmp (key, "ObjectID"))
    value = upnp_get_string (ar, "ContainerID");

  if (!value)
    return 0;

  val = atoi (value);
  free (value);

  return val;
}
