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
#include <string.h>

#include "dlna.h"
#include "profiles.h"

static dlna_registered_profile_t *first_profile = NULL;

extern dlna_registered_profile_t dlna_profile_image_jpeg;
extern dlna_registered_profile_t dlna_profile_image_png;
extern dlna_registered_profile_t dlna_profile_audio_ac3;
extern dlna_registered_profile_t dlna_profile_audio_amr;
extern dlna_registered_profile_t dlna_profile_audio_mp3;

static void
dlna_register_profile (dlna_registered_profile_t *profile)
{
  dlna_registered_profile_t **p;
  
  p = &first_profile;
  while (*p != NULL)
  {
    if ((*p)->id == profile->id)
      return; /* already registered */
    p = &(*p)->next;
  }
  *p = profile;
  profile->next = NULL;
}

void
dlna_register_all_media_profiles (void)
{
  dlna_register_profile (&dlna_profile_image_jpeg);
  dlna_register_profile (&dlna_profile_image_png);
  dlna_register_profile (&dlna_profile_audio_ac3);
  dlna_register_profile (&dlna_profile_audio_amr);
  dlna_register_profile (&dlna_profile_audio_mp3);
}

void
dlna_register_media_profile (dlna_media_profile_t profile)
{
  switch (profile)
  {
  case DLNA_PROFILE_IMAGE_JPEG:
    dlna_register_profile (&dlna_profile_image_jpeg);
    break;
  case DLNA_PROFILE_IMAGE_PNG:
    dlna_register_profile (&dlna_profile_image_png);
    break;
  case DLNA_PROFILE_AUDIO_AC3:
    dlna_register_profile (&dlna_profile_audio_ac3);
    break;
  case DLNA_PROFILE_AUDIO_AMR:
    dlna_register_profile (&dlna_profile_audio_amr);
    break;
  case DLNA_PROFILE_AUDIO_MP3:
    dlna_register_profile (&dlna_profile_audio_mp3);
    break;
  default:
    break;
  }
}

void
dlna_init (void)
{
  /* register all FFMPEG demuxers */
  av_register_all ();
}

dlna_profile_t *
dlna_guess_media_profile (const char *filename)
{
  AVFormatContext *pFormatCtx;
  dlna_registered_profile_t *p;
  dlna_profile_t *profile = NULL;

  if (av_open_input_file (&pFormatCtx, filename, NULL, 0, NULL) != 0)
  {
    printf ("can't open file: %s\n", filename);
    return NULL;
  }

  if (av_find_stream_info (pFormatCtx) < 0)
  {
    printf ("can't find stream info\n");
    return NULL;
  }

  p = first_profile;
  while (p)
  {
    dlna_profile_t *prof = p->probe (pFormatCtx);
    if (prof)
    {
      profile = prof;
      break;
    }
    p = p->next;
  }

  av_close_input_file (pFormatCtx);
  return profile;
}

char *
get_file_extension (const char *filename)
{
  char *str = NULL;

  str = strrchr (filename, '.');
  if (str)
    str++;

  return str;
}

int
match_file_extension (const char *filename, const char *extensions)
{
  const char *ext, *p;
  char ext1[32], *q;

  if (!filename)
    return 0;

  ext = strrchr (filename, '.');
  if (ext)
  {
    ext++;
    p = extensions;
    for (;;)
    {
      q = ext1;
      while (*p != '\0' && *p != ',' && (q - ext1 < sizeof (ext1) - 1))
        *q++ = *p++;
      *q = '\0';
      if (!strcasecmp (ext1, ext))
        return 1;
      if (*p == '\0')
        break;
      p++;
    }
  }
  
  return 0;
}

dlna_profile_t *
set_profile (dlna_profile_t *profile)
{
  dlna_profile_t *p;
  p = malloc (sizeof (dlna_profile_t *));
  memcpy (p, profile, sizeof (*profile));
  return p;
}
