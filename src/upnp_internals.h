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

#ifndef _UPNP_INTERNALS_H_
#define _UPNP_INTERNALS_H_

#include "dlna_internals.h"

#define UPNP_MAX_CONTENT_LENGTH 4096
#define VIRTUAL_DIR "/web"

/* Conection Manager Service */
#define CMS_URL              "cms.xml"
#define CMS_CONTROL_URL      "cms_control"
#define CMS_EVENT_URL        "cms_event"

/* Content Directory Service */
#define CDS_URL              "cms.xml"
#define CDS_CONTROL_URL      "cms_control"
#define CDS_EVENT_URL        "cms_event"


typedef struct upnp_service_s         upnp_service_t;
typedef struct upnp_action_event_s    upnp_action_event_t;
typedef struct upnp_service_action_s  upnp_service_action_t;

struct upnp_action_event_s {
  struct Upnp_Action_Request *ar;
  int status;
  upnp_service_t *service;
};

struct upnp_service_action_s {
  char *name;
  int (*cb) (dlna_t *, upnp_action_event_t *);
};

struct upnp_service_s {
  char *id;
  char *type;
  upnp_service_action_t *actions;
};

struct UpnpVirtualDirCallbacks virtual_dir_callbacks;

int upnp_init (dlna_t *dlna, dlna_device_type_t type);
int upnp_uninit (dlna_t *dlna);

#endif /* _UPNP_INTERNALS_H_ */
