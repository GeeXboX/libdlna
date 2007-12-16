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
#include <stdlib.h>

#include "dlna_internals.h"

void
dlna_set_device_friendly_name (dlna_t *dlna, char *str)
{
  if (!dlna || !str)
    return;

  if (dlna->friendly_name)
    free (dlna->friendly_name);
  dlna->friendly_name = strdup (str);
}

void
dlna_set_device_manufacturer (dlna_t *dlna, char *str)
{
  if (!dlna || !str)
    return;

  if (dlna->manufacturer)
    free (dlna->manufacturer);
  dlna->manufacturer = strdup (str);
}

void
dlna_set_device_manufacturer_url (dlna_t *dlna, char *str)
{
  if (!dlna || !str)
    return;

  if (dlna->manufacturer_url)
    free (dlna->manufacturer_url);
  dlna->manufacturer_url = strdup (str);
}

void
dlna_set_device_model_description (dlna_t *dlna, char *str)
{
  if (!dlna || !str)
    return;

  if (dlna->model_description)
    free (dlna->model_description);
  dlna->model_description = strdup (str);
}

void
dlna_set_device_model_name (dlna_t *dlna, char *str)
{
  if (!dlna || !str)
    return;

  if (dlna->model_name)
    free (dlna->model_name);
  dlna->model_name = strdup (str);
}

void
dlna_set_device_model_number (dlna_t *dlna, char *str)
{
  if (!dlna || !str)
    return;

  if (dlna->model_number)
    free (dlna->model_number);
  dlna->model_number = strdup (str);
}

void
dlna_set_device_serial_number (dlna_t *dlna, char *str)
{
  if (!dlna || !str)
    return;

  if (dlna->serial_number)
    free (dlna->serial_number);
  dlna->serial_number = strdup (str);
}

void
dlna_set_device_uuid (dlna_t *dlna, char *str)
{
  if (!dlna || !str)
    return;

  if (dlna->uuid)
    free (dlna->uuid);
  dlna->uuid = strdup (str);
}
