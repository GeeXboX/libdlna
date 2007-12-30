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
#include <limits.h>

#include "upnp_internals.h"

#ifndef MAX
#define MAX(a,b) ((a) > (b) ? (a) : (b))
#endif

#ifndef MIN
#define MIN(a,b) ((a) < (b) ? (a) : (b))
#endif

typedef enum {
  HTTP_ERROR = -1,
  HTTP_OK    =  0,
} http_error_code_t;

typedef struct http_file_handler_s {
  char *fullpath;
  off_t pos;
  enum {
    HTTP_FILE_LOCAL,
    HTTP_FILE_MEMORY
  } type;
  union {
    struct {
      int fd;
    } local;
    struct {
      char *content;
      off_t len;
    } memory;
  } detail;
} http_file_handler_t;

static inline void
set_service_http_info (struct File_Info *info,
                       const size_t length,
                       const char *content_type)
{
  info->file_length   = length;
  info->last_modified = 0;
  info->is_directory  = 0;
  info->is_readable   = 1;
  info->content_type  = ixmlCloneDOMString (content_type);
}

static int
upnp_http_get_info (void *cookie,
                    const char *filename,
                    struct File_Info *info)
{
  dlna_t *dlna;

  if (!cookie || !filename || !info)
    return HTTP_ERROR;

  dlna = (dlna_t *) cookie;

  dlna_log (dlna, DLNA_MSG_INFO,
            "%s, filename : %s\n", __FUNCTION__, filename);

  /* ask for Content Directory Service (CDS) */
  if (!strcmp (filename, CDS_LOCATION))
  {
    set_service_http_info (info, CDS_DESCRIPTION_LEN, SERVICE_CONTENT_TYPE);
    return HTTP_OK;
  }

  /* ask for Connection Manager Service (CMS) */
  if (!strcmp (filename, CMS_LOCATION))
  {
    set_service_http_info (info, CMS_DESCRIPTION_LEN, SERVICE_CONTENT_TYPE);
    return HTTP_OK;
  }

  /* ask for AVTransport Service (AVTS) */
  if (!strcmp (filename, AVTS_LOCATION))
  {
    set_service_http_info (info, AVTS_DESCRIPTION_LEN, SERVICE_CONTENT_TYPE);
    return HTTP_OK;
  }

  /* ask for anything else ... */
  return HTTP_ERROR; /* not yet implemented so just bails out right now */
  
  return HTTP_OK;
}

static UpnpWebFileHandle
http_get_file_from_memory (const char *fullpath,
                           const char *description,
                           const size_t length)
{
  http_file_handler_t *hdl;

  if (!fullpath || !description || length == 0)
    return NULL;
  
  hdl                        = malloc (sizeof (http_file_handler_t));
  hdl->fullpath              = strdup (fullpath);
  hdl->pos                   = 0;
  hdl->type                  = HTTP_FILE_MEMORY;
  hdl->detail.memory.content = strdup (description);
  hdl->detail.memory.len     = length;

  return ((UpnpWebFileHandle) hdl);
}

static UpnpWebFileHandle
upnp_http_open (void *cookie,
                const char *filename,
                enum UpnpOpenFileMode mode)
{
  dlna_t *dlna;

  if (!cookie || !filename)
    return NULL;

  dlna = (dlna_t *) cookie;

  dlna_log (dlna, DLNA_MSG_INFO,
            "%s, filename : %s\n", __FUNCTION__, filename);

  if (mode != UPNP_READ)
    return NULL;

  /* ask for Content Directory Service (CDS) */
  if (!strcmp (filename, CDS_LOCATION))
    return http_get_file_from_memory (CDS_LOCATION,
                                      CDS_DESCRIPTION, CDS_DESCRIPTION_LEN);
  
  /* ask for Connection Manager Service (CMS) */
  if (!strcmp (filename, CMS_LOCATION))
    return http_get_file_from_memory (CMS_LOCATION,
                                      CMS_DESCRIPTION, CMS_DESCRIPTION_LEN);
  
  /* ask for AVTransport Service (AVTS) */
  if (!strcmp (filename, AVTS_LOCATION))
    return http_get_file_from_memory (AVTS_LOCATION,
                                      AVTS_DESCRIPTION, AVTS_DESCRIPTION_LEN);
  
  /* ask for anything else ... */
  /* not yet implemented so just bails out right now */
  
  return NULL;
}

static int
upnp_http_read (void *cookie,
                UpnpWebFileHandle fh,
                char *buf,
                size_t buflen)
{
  dlna_t *dlna;
  http_file_handler_t *hdl;
  ssize_t len = -1;

  if (!cookie || !fh)
    return HTTP_ERROR;

  dlna = (dlna_t *) cookie;
  hdl = (http_file_handler_t *) fh;
  
  dlna_log (dlna, DLNA_MSG_INFO, "%s\n", __FUNCTION__);

  switch (hdl->type)
  {
  case HTTP_FILE_LOCAL:
    break; /* not yet implemented */
  case HTTP_FILE_MEMORY:
    dlna_log (dlna, DLNA_MSG_INFO, "Read file from memory.\n");
    len = (ssize_t) MIN (buflen, hdl->detail.memory.len - hdl->pos);
    memcpy (buf, hdl->detail.memory.content + hdl->pos, (ssize_t) len);
    break;
  default:
    dlna_log (dlna, DLNA_MSG_ERROR, "Unknown HTTP file type.\n");
    break;
  }

  if (len > 0)
    hdl->pos += len;

  dlna_log (dlna, DLNA_MSG_INFO, "Read %zd bytes.\n", len);

  return len;
}

static int
upnp_http_write (void *cookie dlna_unused,
                 UpnpWebFileHandle fh dlna_unused,
                 char *buf dlna_unused,
                 size_t buflen dlna_unused)
{
  return 0;
}

static int
upnp_http_seek (void *cookie,
                UpnpWebFileHandle fh,
                off_t offset,
                int origin)
{
  dlna_t *dlna;
  http_file_handler_t *hdl;
  off_t newpos = -1;
  
  if (!cookie || !fh)
    return HTTP_ERROR;

  dlna = (dlna_t *) cookie;
  hdl = (http_file_handler_t *) fh;
  
  dlna_log (dlna, DLNA_MSG_INFO, "%s\n", __FUNCTION__);

  switch (origin)
  {
  case SEEK_SET:
    dlna_log (dlna, DLNA_MSG_INFO,
              "Attempting to seek to %lld (was at %lld) in %s\n",
              offset, hdl->pos, hdl->fullpath);
    newpos = offset;
    break;
  case SEEK_CUR:
    dlna_log (dlna, DLNA_MSG_INFO,
              "Attempting to seek by %lld from %lld in %s\n",
              offset, hdl->pos, hdl->fullpath);
    newpos = hdl->pos + offset;
    break;
  case SEEK_END:
    dlna_log (dlna, DLNA_MSG_INFO,
              "Attempting to seek by %lld from end (was at %lld) in %s\n",
              offset, hdl->pos, hdl->fullpath);

    if (hdl->type == HTTP_FILE_LOCAL)
    {
      /* not yet implemented */
      return HTTP_ERROR;
    }
    else if (hdl->type == HTTP_FILE_MEMORY)
      newpos = hdl->detail.memory.len + offset;
    break;
  }

  switch (hdl->type)
  {
  case HTTP_FILE_LOCAL:
    /* not yet implemented */
    return HTTP_ERROR;
  case HTTP_FILE_MEMORY:
    if (newpos < 0 || newpos > hdl->detail.memory.len)
    {
      dlna_log (dlna, DLNA_MSG_ERROR,
                "%s: cannot seek: %s\n", hdl->fullpath, strerror (EINVAL));
      return HTTP_ERROR;
    }
    break;
  }

  /* everything went well ... */
  hdl->pos = newpos;
  return HTTP_OK;
}

static int
upnp_http_close (void *cookie,
                 UpnpWebFileHandle fh)
{
  dlna_t *dlna;
  http_file_handler_t *hdl;
  
  if (!cookie || !fh)
    return HTTP_ERROR;

  dlna = (dlna_t *) cookie;
  hdl = (http_file_handler_t *) fh;
  
  dlna_log (dlna, DLNA_MSG_INFO, "%s\n", __FUNCTION__);

  switch (hdl->type)
  {
  case HTTP_FILE_LOCAL:
    /* not yet implemented */
    break;
  case HTTP_FILE_MEMORY:
    /* no close operation is needed, just free file content */
    if (hdl->detail.memory.content)
      free (hdl->detail.memory.content);
    break;
  default:
    dlna_log (dlna, DLNA_MSG_ERROR, "Unknown HTTP file type.\n");
    break;
  }

  if (hdl->fullpath)
    free (hdl->fullpath);
  free (hdl);

  return HTTP_OK;
}

struct UpnpVirtualDirCallbacks virtual_dir_callbacks = {
  NULL,
  upnp_http_get_info,
  upnp_http_open,
  upnp_http_read,
  upnp_http_write,
  upnp_http_seek,
  upnp_http_close
};
