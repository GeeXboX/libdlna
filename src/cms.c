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

#include "upnp_internals.h"

/* Represent the CMS GetCurrentConnectionInfo action. */
#define SERVICE_CMS_ACTION_CON_INFO "GetCurrentConnectionInfo"

/* Represent the CMS GetProtocolInfo action. */
#define SERVICE_CMS_ACTION_PROT_INFO "GetProtocolInfo"

/* Represent the CMS GetCurrentConnectionIDs action. */
#define SERVICE_CMS_ACTION_CON_ID "GetCurrentConnectionIDs"

#define SERVICE_CMS_ACTION_PREPARE "PrepareForConnection"
#define SERVICE_CMS_ACTION_CON_COMPLETE "ConnectionComplete"


/* List of UPnP ConnectionManager Service actions */
upnp_service_action_t cms_service_actions[] = {
  { SERVICE_CMS_ACTION_CON_INFO,  NULL },
  { SERVICE_CMS_ACTION_PROT_INFO, NULL },
  { SERVICE_CMS_ACTION_CON_ID,    NULL },
  { SERVICE_CMS_ACTION_PREPARE,    NULL },
  { SERVICE_CMS_ACTION_CON_COMPLETE, NULL },
  { NULL,                         NULL }
};
