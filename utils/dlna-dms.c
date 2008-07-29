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
#include <stdio.h>
#include <libgen.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>

#include "dlna.h"

static void
add_dir (dlna_t *dlna, char *dir, uint32_t id)
{
  struct dirent **namelist;
  int n, i;

  n = scandir (dir, &namelist, 0, alphasort);
  if (n < 0)
  {
    perror ("scandir");
    return;
  }

  for (i = 0; i < n; i++)
  {
    struct stat st;
    char *fullpath = NULL;

    if (namelist[i]->d_name[0] == '.')
    {
      free (namelist[i]);
      continue;
    }

    fullpath = malloc (strlen (dir) + strlen (namelist[i]->d_name) + 2);
    sprintf (fullpath, "%s/%s", dir, namelist[i]->d_name);

    if (stat (fullpath, &st) < 0)
    {
      free (namelist[i]);
      free (fullpath);
      continue;
    }

    if (S_ISDIR (st.st_mode))
    {
      uint32_t cid;
      cid = dlna_vfs_add_container (dlna, basename (fullpath), 0, id);
      add_dir (dlna, fullpath, cid);
    }
    else
      dlna_vfs_add_resource (dlna, basename (fullpath),
                             fullpath, st.st_size, id);
    
    free (namelist[i]);
    free (fullpath);
  }
  free (namelist);
}

int
main (int argc, char **argv)
{
  dlna_t *dlna;
  dlna_org_flags_t flags;
  int c;
  
  printf ("libdlna Digital Media Server (DMS) API example\n");
  printf ("Using %s\n", LIBDLNA_IDENT);

  flags = DLNA_ORG_FLAG_STREAMING_TRANSFER_MODE |
    DLNA_ORG_FLAG_BACKGROUND_TRANSFERT_MODE |
    DLNA_ORG_FLAG_CONNECTION_STALL |
    DLNA_ORG_FLAG_DLNA_V15;
  
  /* init DLNA stack */
  dlna = dlna_init ();
  dlna_set_org_flags (dlna, flags);
  dlna_set_verbosity (dlna, DLNA_MSG_INFO);
  dlna_set_capability_mode (dlna, DLNA_CAPABILITY_UPNP_AV);
  dlna_set_extension_check (dlna, 1);
  dlna_register_all_media_profiles (dlna);

  /* define NIC to be used */
  dlna_set_interface (dlna, "eth0");

  /* set some UPnP device properties */
  dlna_device_set_friendly_name (dlna, "libdlna DMS template");
  dlna_device_set_uuid (dlna, "123456789");

  /* initialize DMS: from this point you have a working/running media server */
  if (dlna_dms_init (dlna) != DLNA_ST_OK)
  {
    printf ("DMS init went wrong\n");
    dlna_uninit (dlna);
    return -1;
  }

  if (argc > 1)
  {
    int i;

    for (i = 1; i < argc; i++)
    {
      struct stat st;

      if (stat (argv[i], &st) < 0)
        continue;
      
      printf ("Trying to share '%s'\n", argv[i]);
      if (S_ISDIR (st.st_mode))
        add_dir (dlna, argv[i], 0);
      else
        dlna_vfs_add_resource (dlna, basename (argv[i]),
                               argv[i], st.st_size, 0);
    }
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
