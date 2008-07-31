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

#ifndef UPNP_INTERNALS_H
#define UPNP_INTERNALS_H

#include "dlna_internals.h"
#include "buffer.h"
#include "cms.h"
#include "cds.h"
#include "avts.h"
#include "msr.h"

#define SERVICE_CONTENT_TYPE "text/xml"
#define DLNA_MAX_CONTENT_LENGTH 4096
#define VIRTUAL_DIR "/web"
#define SERVICES_VIRTUAL_DIR "/services"
#define XBOX_MODEL_NAME "Windows Media Connect Compatible"

struct dlnaVirtualDirCallbacks virtual_dir_callbacks;

int upnp_init (dlna_t *dlna, dlna_device_type_t type);
int upnp_uninit (dlna_t *dlna);

int upnp_add_response (upnp_action_event_t *ev, char *key, const char *value);
char *upnp_get_string (struct dlna_Action_Request *ar, const char *key);
int upnp_get_ui4 (struct dlna_Action_Request *ar, const char *key);

#endif /* UPNP_INTERNALS_H */
