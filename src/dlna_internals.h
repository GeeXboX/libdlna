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

#ifndef _DLNA_INTERNALS_H_
#define _DLNA_INTERNALS_H_

#if defined(__GNUC__)
#    define dlna_unused __attribute__((unused))
#else
#    define dlna_unused
#endif

#include "dlna.h"

#include <upnp/upnp.h>
#include <upnp/upnptools.h>

typedef enum {
  DLNA_DEVICE_UNKNOWN,
  DLNA_DEVICE_DMS,      /* Digital Media Server */
  DLNA_DEVICE_DMP,      /* Digital Media Player */
} dlna_device_type_t;

/**
 * DLNA Library's controller.
 * This controls the whole library.
 */
struct dlna_s {
  /* has the library's been inited */
  int inited;
  /* defines verbosity level */
  dlna_verbosity_level_t verbosity;
  /* defines flexibility on file extension's check */
  int check_extensions;
  /* linked-list of registered DLNA profiles */
  void *first_profile;

  /* UPnP Properties */
  char *interface;
  unsigned short port; /* server port */
  UpnpDevice_Handle dev;
  char *friendly_name;
  char *manufacturer;
  char *manufacturer_url;
  char *model_description;
  char *model_name;
  char *model_number;
  char *model_url;
  char *serial_number;
  char *uuid;
};

void dlna_log (dlna_t *dlna,
               dlna_verbosity_level_t level,
               const char *format, ...);

#endif /* _DLNA_INTERNALS_H_ */
