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

#include "dlna_internals.h"
#include "upnp_internals.h"

#define DLNA_DMS_DESCRIPTION_HEADER \
"<?xml version=\"1.0\" encoding=\"utf-8\"?>" \
"<root xmlns=\"urn:schemas-upnp-org:device-1-0\">" \
"  <specVersion>" \
"    <major>1</major>" \
"    <minor>0</minor>" \
"  </specVersion>" \
"  <device>" \
"    <deviceType>urn:schemas-upnp-org:device:MediaServer:1</deviceType>" \
"    <friendlyName>%s: 1</friendlyName>" \
"    <manufacturer>%s</manufacturer>" \
"    <manufacturerURL>%s</manufacturerURL>" \
"    <modelDescription>%s</modelDescription>" \
"    <modelName>%s</modelName>" \
"    <modelNumber>%s</modelNumber>" \
"    <modelURL>%s</modelURL>" \
"    <serialNumber>%s</serialNumber>" \
"    <UDN>uuid:%s</UDN>" \
"    <presentationURL>%s/%s</presentationURL>" \
"    <dlna:X_DLNADOC xmlns:dlna=\"urn:schemas-dlna-org:device-1-0\">DMS-1.00</dlna:X_DLNADOC>" \
"    <serviceList>"

#define DLNA_DMS_DESCRIPTION_FOOTER \
"    </serviceList>" \
"  </device>" \
"</root>"

#define DLNA_SERVICE_DESCRIPTION \
"      <service>" \
"        <serviceType>%s</serviceType>" \
"        <serviceId>%s</serviceId>" \
"        <SCPDURL>%s/%s</SCPDURL>" \
"        <controlURL>%s/%s</controlURL>" \
"        <eventSubURL>%s/%s</eventSubURL>" \
"      </service>" \

char *
dlna_dms_description_get (dlna_t *dlna)
{
  buffer_t *b = NULL;
  char *model_name, *desc = NULL;
  upnp_service_t *service;
  
  if (!dlna)
    return NULL;

  if (dlna->mode == DLNA_CAPABILITY_UPNP_AV_XBOX)
  {
    model_name =
      malloc (strlen (XBOX_MODEL_NAME) + strlen (dlna->model_name) + 4);
    sprintf (model_name, "%s (%s)", XBOX_MODEL_NAME, dlna->model_name);
  }
  else
    model_name = strdup (dlna->model_name);

  b = buffer_new ();
  
  buffer_appendf (b, DLNA_DMS_DESCRIPTION_HEADER, dlna->friendly_name,
                  dlna->manufacturer, dlna->manufacturer_url,
                  dlna->model_description, model_name,
                  dlna->model_number, dlna->model_url,
                  dlna->serial_number, dlna->uuid,
                  SERVICES_VIRTUAL_DIR, dlna->presentation_url);

  free (model_name);
  
  for (service = dlna->services; service; service = service->hh.next)
    buffer_appendf (b, DLNA_SERVICE_DESCRIPTION,
                    service->type, service->id,
                    SERVICES_VIRTUAL_DIR, service->scpd_url,
                    SERVICES_VIRTUAL_DIR, service->control_url,
                    SERVICES_VIRTUAL_DIR, service->event_url);

  buffer_append (b, DLNA_DMS_DESCRIPTION_FOOTER);

  desc = strdup (b->buf);
  buffer_free (b);
  
  return desc;

}

int
dlna_dms_init (dlna_t *dlna)
{
  if (!dlna)
    return DLNA_ST_ERROR;

  if (!dlna->inited)
    return DLNA_ST_ERROR;

  dlna_service_register (dlna, DLNA_SERVICE_CONNECTION_MANAGER);
  dlna_service_register (dlna, DLNA_SERVICE_CONTENT_DIRECTORY);
  dlna_service_register (dlna, DLNA_SERVICE_AV_TRANSPORT);
  if (dlna->mode == DLNA_CAPABILITY_UPNP_AV_XBOX)
    dlna_service_register (dlna, DLNA_SERVICE_MS_REGISTAR);
  
  return upnp_init (dlna, DLNA_DEVICE_DMS);
}

int
dlna_dms_uninit (dlna_t *dlna)
{
  if (!dlna)
    return DLNA_ST_ERROR;

  if (!dlna->inited)
    return DLNA_ST_ERROR;

  return upnp_uninit (dlna);
}
