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

  dlna_log (dlna, DLNA_MSG_INFO, "%s:%d\n", __FUNCTION__, __LINE__);

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

  dlna_log (dlna, DLNA_MSG_INFO, "%s:%d\n", __FUNCTION__, __LINE__);

  return ev->status;
}

static int
cms_get_current_connection_info (dlna_t *dlna, upnp_action_event_t *ev)
{
  if (!dlna || !ev)
    return 0;

  dlna_log (dlna, DLNA_MSG_INFO, "%s:%d\n", __FUNCTION__, __LINE__);

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
