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

#include "dlna_internals.h"
#include "upnp_internals.h"

#define UPNP_DMS_DESCRIPTION \
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
"    <serviceList>" \
"      <service>" \
"        <serviceType>urn:schemas-upnp-org:service:ConnectionManager:1</serviceType>" \
"        <serviceId>urn:upnp-org:serviceId:ConnectionManager</serviceId>" \
"        <SCPDURL>%s/%s</SCPDURL>" \
"        <controlURL>%s/%s</controlURL>" \
"        <eventSubURL>%s/%s</eventSubURL>" \
"      </service>" \
"      <service>" \
"        <serviceType>urn:schemas-upnp-org:service:ContentDirectory:1</serviceType>" \
"        <serviceId>urn:upnp-org:serviceId:ContentDirectory</serviceId>" \
"        <SCPDURL>%s/%s</SCPDURL>" \
"        <controlURL>%s/%s</controlURL>" \
"        <eventSubURL>%s/%s</eventSubURL>" \
"      </service>" \
"      <service>" \
"        <serviceType>urn:schemas-upnp-org:service:AVTransport:1</serviceType>" \
"        <serviceId>urn:upnp-org:serviceId:AVTransport</serviceId>" \
"        <SCPDURL>%s/%s</SCPDURL>" \
"        <controlURL>%s/%s</controlURL>" \
"        <eventSubURL>%s/%s</eventSubURL>" \
"      </service>" \
"    </serviceList>" \
"  </device>" \
"</root>"

char *
dlna_dms_description_get (const char *friendly_name,
                          const char *manufacturer,
                          const char *manufacturer_url,
                          const char *model_description,
                          const char *model_name,
                          const char *model_number,
                          const char *model_url,
                          const char *serial_number,
                          const char *uuid,
                          const char *presentation_url)
{
  char *desc = NULL;
  size_t len;

  if (!friendly_name || !manufacturer || !manufacturer_url ||
      !model_description || !model_name || !model_number ||
      !model_url || !serial_number || !uuid || !presentation_url)
    return NULL;
  
  len = strlen (UPNP_DMS_DESCRIPTION) + strlen (friendly_name)
    + strlen (manufacturer) + strlen (manufacturer_url)
    + strlen (model_description) + strlen (model_name)
    + strlen (model_number) + strlen (model_url) + strlen (serial_number)
    + strlen (uuid) +
    + strlen (VIRTUAL_DIR) + strlen (presentation_url) +
    + strlen (VIRTUAL_DIR) + strlen (CMS_URL) +
    + strlen (VIRTUAL_DIR) + strlen (CMS_CONTROL_URL) +
    + strlen (VIRTUAL_DIR) + strlen (CMS_EVENT_URL) +
    + strlen (VIRTUAL_DIR) + strlen (CDS_URL) +
    + strlen (VIRTUAL_DIR) + strlen (CDS_CONTROL_URL) +
    + strlen (VIRTUAL_DIR) + strlen (CDS_EVENT_URL) +
    + strlen (VIRTUAL_DIR) + strlen (AVTS_URL) +
    + strlen (VIRTUAL_DIR) + strlen (AVTS_CONTROL_URL) +
    + strlen (VIRTUAL_DIR) + strlen (AVTS_EVENT_URL) +
    1;

  desc = malloc (len);
  memset (desc, 0, len);
  sprintf (desc, UPNP_DMS_DESCRIPTION, friendly_name,
           manufacturer, manufacturer_url, model_description,
           model_name, model_number, model_url, serial_number, uuid,
           VIRTUAL_DIR, presentation_url,
           VIRTUAL_DIR, CMS_URL,
           VIRTUAL_DIR, CMS_CONTROL_URL,
           VIRTUAL_DIR, CMS_EVENT_URL,
           VIRTUAL_DIR, CDS_URL,
           VIRTUAL_DIR, CDS_CONTROL_URL,
           VIRTUAL_DIR, CDS_EVENT_URL,
           VIRTUAL_DIR, AVTS_URL,
           VIRTUAL_DIR, AVTS_CONTROL_URL,
           VIRTUAL_DIR, AVTS_EVENT_URL);

  return desc;
}

int
dlna_dms_init (dlna_t *dlna)
{
  if (!dlna)
    return DLNA_ST_ERROR;

  if (!dlna->inited)
    return DLNA_ST_ERROR;

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
