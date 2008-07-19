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
 * ContentDirectory service specifications can be found at:
 * http://upnp.org/standardizeddcps/documents/ContentDirectory1.0.pdf
 * http://upnp.org/specs/av/UPnP-av-ContentDirectory-v2-Service-20060531.pdf
 */

#include "upnp_internals.h"

/* CDS Action Names */
#define SERVICE_CDS_ACTION_SEARCH_CAPS        "GetSearchCapabilities"
#define SERVICE_CDS_ACTION_SORT_CAPS          "GetSortCapabilities"
#define SERVICE_CDS_ACTION_UPDATE_ID          "GetSystemUpdateID"
#define SERVICE_CDS_ACTION_BROWSE             "Browse"
#define SERVICE_CDS_ACTION_SEARCH             "Search"
#define SERVICE_CDS_ACTION_CREATE_OBJ         "CreateObject"
#define SERVICE_CDS_ACTION_DESTROY_OBJ        "DestroyObject"
#define SERVICE_CDS_ACTION_UPDATE_OBJ         "UpdateObject"
#define SERVICE_CDS_ACTION_IMPORT_RES         "ImportResource"
#define SERVICE_CDS_ACTION_EXPORT_RES         "ExportResource"
#define SERVICE_CDS_ACTION_STOP_TRANSFER      "StopTransferResource"
#define SERVICE_CDS_ACTION_GET_PROGRESS       "GetTransferProgress"
#define SERVICE_CDS_ACTION_DELETE_RES         "DeleteResource"
#define SERVICE_CDS_ACTION_CREATE_REF         "CreateReference"

static int
cds_get_search_capabilities (dlna_t *dlna, upnp_action_event_t *ev)
{
  if (!dlna || !ev)
    return 0;

  dlna_log (dlna, DLNA_MSG_INFO, "%s:%d\n", __FUNCTION__, __LINE__);

  return ev->status;
}

static int
cds_get_sort_capabilities (dlna_t *dlna, upnp_action_event_t *ev)
{
  if (!dlna || !ev)
    return 0;

  dlna_log (dlna, DLNA_MSG_INFO, "%s:%d\n", __FUNCTION__, __LINE__);

  return ev->status;
}

static int
cds_get_system_update_id (dlna_t *dlna, upnp_action_event_t *ev)
{
  if (!dlna || !ev)
    return 0;

  dlna_log (dlna, DLNA_MSG_INFO, "%s:%d\n", __FUNCTION__, __LINE__);

  return ev->status;
}

static int
cds_browse (dlna_t *dlna, upnp_action_event_t *ev)
{
  if (!dlna || !ev)
    return 0;

  dlna_log (dlna, DLNA_MSG_INFO, "%s:%d\n", __FUNCTION__, __LINE__);

  return ev->status;
}

/* List of UPnP ContentDirectory Service actions */
upnp_service_action_t cds_service_actions[] = {
  { SERVICE_CDS_ACTION_SEARCH_CAPS,    cds_get_search_capabilities },
  { SERVICE_CDS_ACTION_SORT_CAPS,      cds_get_sort_capabilities },
  { SERVICE_CDS_ACTION_UPDATE_ID,      cds_get_system_update_id },
  { SERVICE_CDS_ACTION_BROWSE,         cds_browse },
  { SERVICE_CDS_ACTION_SEARCH,         NULL },
  { SERVICE_CDS_ACTION_CREATE_OBJ,     NULL },
  { SERVICE_CDS_ACTION_DESTROY_OBJ,    NULL },
  { SERVICE_CDS_ACTION_UPDATE_OBJ,     NULL },
  { SERVICE_CDS_ACTION_IMPORT_RES,     NULL },
  { SERVICE_CDS_ACTION_EXPORT_RES,     NULL },
  { SERVICE_CDS_ACTION_STOP_TRANSFER,  NULL },
  { SERVICE_CDS_ACTION_GET_PROGRESS,   NULL },
  { SERVICE_CDS_ACTION_DELETE_RES,     NULL },
  { SERVICE_CDS_ACTION_CREATE_REF,     NULL },
  { NULL,                              NULL }
};
