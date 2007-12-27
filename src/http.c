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

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>

#include "upnp_internals.h"

static int
upnp_http_get_info (const char *filename dlna_unused,
                    struct File_Info *info dlna_unused)
{
  return 0;
}

static UpnpWebFileHandle
upnp_http_open (const char *filename dlna_unused,
                enum UpnpOpenFileMode mode dlna_unused)
{
  return NULL;
}

static int
upnp_http_read (UpnpWebFileHandle fh dlna_unused,
                char *buf dlna_unused,
                size_t buflen dlna_unused)
{
  return 0;
}

static int
upnp_http_write (UpnpWebFileHandle fh dlna_unused,
                 char *buf dlna_unused,
                 size_t buflen dlna_unused)
{
  return 0;
}

static int
upnp_http_seek (UpnpWebFileHandle fh dlna_unused,
                off_t offset dlna_unused,
                int origin dlna_unused)
{
  return 0;
}

static int
upnp_http_close (UpnpWebFileHandle fh dlna_unused)
{
  return 0;
}

struct UpnpVirtualDirCallbacks virtual_dir_callbacks = {
  upnp_http_get_info,
  upnp_http_open,
  upnp_http_read,
  upnp_http_write,
  upnp_http_seek,
  upnp_http_close
};
