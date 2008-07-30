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

/*
 * ConnectionManager service specifications can be found at:
 * http://upnp.org/standardizeddcps/documents/ConnectionManager1.0.pdf
 * http://upnp.org/specs/av/UPnP-av-ConnectionManager-v2-Service-20060531.pdf
 */

#include "upnp_internals.h"

/* CMS Action Names */
#define SERVICE_CMS_ACTION_PROT_INFO          "GetProtocolInfo"
#define SERVICE_CMS_ACTION_PREPARE            "PrepareForConnection"
#define SERVICE_CMS_ACTION_CON_COMPLETE       "ConnectionComplete"
#define SERVICE_CMS_ACTION_CON_ID             "GetCurrentConnectionIDs"
#define SERVICE_CMS_ACTION_CON_INFO           "GetCurrentConnectionInfo"

/* CMS Arguments */
#define SERVICE_CMS_ARG_SOURCE                "Source"
#define SERVICE_CMS_ARG_SINK                  "Sink"
#define SERVICE_CMS_ARG_CONNECTION_IDS        "ConnectionIDs"
#define SERVICE_CMS_ARG_CONNECTION_ID         "ConnectionID"
#define SERVICE_CMS_ARG_RCS_ID                "RcsID"
#define SERVICE_CMS_ARG_TRANSPORT_ID          "AVTransportID"
#define SERVICE_CMS_ARG_PROT_INFO             "ProtocolInfo"
#define SERVICE_CMS_ARG_PEER_CON_MANAGER      "PeerConnectionManager"
#define SERVICE_CMS_ARG_PEER_CON_ID           "PeerConnectionID"
#define SERVICE_CMS_ARG_DIRECTION             "Direction"
#define SERVICE_CMS_ARG_STATUS                "Status"

/* CMS Argument Values */
#define SERVICE_CMS_DEFAULT_CON_ID            "0"
#define SERVICE_CMS_UNKNOW_ID                 "-1"
#define SERVICE_CMS_OUTPUT                    "Output"
#define SERVICE_CMS_STATUS_OK                 "OK"

/* CMS Error Codes */
#define SERVICE_CMS_ERR_INVALID_ARGS          402
#define SERVICE_CMS_ERR_PARAMETER_MISMATCH    706

/*
 * GetProtocolInfo:
 *   Returns the protocol-related info that this ConnectionManager supports in
 *   its current state, as a comma-separate list of strings.
 */
static int
cms_get_protocol_info (dlna_t *dlna, upnp_action_event_t *ev)
{
  char **mimes, **tmp;
  buffer_t *source;
  
  if (!dlna || !ev)
    return 0;

  source = buffer_new ();
  mimes = dlna_get_supported_mime_types (dlna);
  tmp = mimes;

  while (*tmp)
  {
    /* we do only support HTTP right now */
    /* format for protocol info is:
     *  <protocol>:<network>:<contentFormat>:<additionalInfo>
     */
    buffer_appendf (source, "http-get:*:%s:*", *tmp++);
    if (*tmp)
      buffer_append (source, ",");
  }

  upnp_add_response (ev, SERVICE_CMS_ARG_SOURCE, source->buf);
  upnp_add_response (ev, SERVICE_CMS_ARG_SINK, "");
  
  buffer_free (source);
  
  return ev->status;
}

static int
cms_get_current_connection_ids (dlna_t *dlna, upnp_action_event_t *ev)
{
  if (!dlna || !ev)
    return 0;

  upnp_add_response (ev, SERVICE_CMS_ARG_CONNECTION_IDS, "");
  
  return ev->status;
}

static int
cms_get_current_connection_info (dlna_t *dlna, upnp_action_event_t *ev)
{
  char **mimes, **tmp;
  
  if (!dlna || !ev)
    return 0;

  upnp_add_response (ev, SERVICE_CMS_ARG_CONNECTION_ID,
                     SERVICE_CMS_DEFAULT_CON_ID);
  upnp_add_response (ev, SERVICE_CMS_ARG_RCS_ID, SERVICE_CMS_UNKNOW_ID);
  upnp_add_response (ev, SERVICE_CMS_ARG_TRANSPORT_ID, SERVICE_CMS_UNKNOW_ID);

  mimes = dlna_get_supported_mime_types (dlna);
  tmp = mimes;

  while (*tmp)
  {
    char protocol[512];

    memset (protocol, '\0', sizeof (protocol));
    snprintf (protocol, sizeof (protocol), "http-get:*:%s:*", *tmp++);
    upnp_add_response (ev, SERVICE_CMS_ARG_PROT_INFO, protocol);
  }
  
  upnp_add_response (ev, SERVICE_CMS_ARG_PEER_CON_MANAGER, "");
  upnp_add_response (ev, SERVICE_CMS_ARG_PEER_CON_ID, SERVICE_CMS_UNKNOW_ID);
  upnp_add_response (ev, SERVICE_CMS_ARG_DIRECTION, SERVICE_CMS_OUTPUT);
  upnp_add_response (ev, SERVICE_CMS_ARG_STATUS, SERVICE_CMS_STATUS_OK);
  
  return ev->status;
}

/* List of UPnP ConnectionManager Service actions */
upnp_service_action_t cms_service_actions[] = {
  { SERVICE_CMS_ACTION_PROT_INFO,     cms_get_protocol_info },
  { SERVICE_CMS_ACTION_PREPARE,       NULL },
  { SERVICE_CMS_ACTION_CON_COMPLETE,  NULL },
  { SERVICE_CMS_ACTION_CON_ID,        cms_get_current_connection_ids },
  { SERVICE_CMS_ACTION_CON_INFO,      cms_get_current_connection_info },
  { NULL,                             NULL }
};

