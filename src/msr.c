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

#include "upnp_internals.h"

/* MSR Action Names */
#define SERVICE_MSR_ACTION_IS_AUTHORIZED            "IsAuthorized"
#define SERVICE_MSR_ACTION_REGISTER_DEVICE          "RegisterDevice"
#define SERVICE_MSR_ACTION_IS_VALIDATED             "IsValidated"

/* MSR Arguments */
#define SERVICE_MSR_ARG_DEVICE_ID                   "DeviceID"
#define SERVICE_MSR_ARG_RESULT                      "Result"
#define SERVICE_MSR_ARG_REGISTRATION_REQUEST_MSG    "RegistrationReqMsg"
#define SERVICE_MSR_ARG_REGISTRATION_RESPONSE_MSG   "RegistrationRespMsg"

/* MSR Argument Values */
#define SERVICE_MSR_STATUS_OK                       "1"

static int
msr_is_authorized (dlna_t *dlna, upnp_action_event_t *ev)
{
  if (!dlna || !ev)
    return 0;

  /* send a fake authorization to these stupid MS players ;-) */
  upnp_add_response (ev, SERVICE_MSR_ARG_RESULT, SERVICE_MSR_STATUS_OK);

  return ev->status;
}

static int
msr_register_device (dlna_t *dlna, upnp_action_event_t *ev)
{
  if (!dlna || !ev)
    return 0;

  /* no action is needed */

  return ev->status;
}

static int
msr_is_validated (dlna_t *dlna, upnp_action_event_t *ev)
{
  if (!dlna || !ev)
    return 0;

  /* send a fake validation to these stupid MS players ;-) */
  upnp_add_response (ev, SERVICE_MSR_ARG_RESULT, SERVICE_MSR_STATUS_OK);

  return ev->status;
}

/* List of UPnP Microsoft Registrar Service actions */
upnp_service_action_t msr_service_actions[] = {
  { SERVICE_MSR_ACTION_IS_AUTHORIZED,   msr_is_authorized },
  { SERVICE_MSR_ACTION_REGISTER_DEVICE, msr_register_device },
  { SERVICE_MSR_ACTION_IS_VALIDATED,    msr_is_validated },
  { NULL,                               NULL }
};
