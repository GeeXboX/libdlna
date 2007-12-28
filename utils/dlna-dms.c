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

#include <stdlib.h>
#include <stdio.h>

#include "dlna.h"

int
main (int argc, char **argv)
{
  dlna_t *dlna;
  int c;
  argc = 0;
  argv = NULL;
  
  printf ("libdlna Digital Media Server (DMS) API example\n");
  printf ("Using %s\n", LIBDLNA_IDENT);

  /* init DLNA stack */
  dlna = dlna_init ();
  dlna_set_verbosity (dlna, DLNA_MSG_INFO);
  dlna_set_extension_check (dlna, 1);
  dlna_register_all_media_profiles (dlna);

  /* define NIC to be used */
  dlna_set_interface (dlna, "eth0");

  /* set some UPnP device properties */
  dlna_set_device_friendly_name (dlna, "libdlna DMS template");
  dlna_set_device_uuid (dlna, "123456789");

  /* initialize DMS: from this point you have a working/running media server */
  if (dlna_dms_init (dlna) != DLNA_ST_OK)
  {
    printf ("DMS init went wrong\n");
    dlna_uninit (dlna);
    return -1;
  }

  printf ("Hit 'q' or 'Q' + Enter to shutdown\n");
  while (1)
  {
    c = getchar ();
    if (c == 'q' || c == 'Q')
      break;
  }
  
  /* DMS shutdown */
  dlna_dms_uninit (dlna);

  /* DLNA stack cleanup */
  dlna_uninit (dlna);
  
  return 0;
}
