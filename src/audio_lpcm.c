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

#define LPCM_KNOWN_EXTENSIONS "pcm,lpcm,wav,aiff"
#define LPCM_MIME_TYPE "audio/L16"
#define LPCM_LABEL "2-ch"

/* Profile for audio media class content */
static dlna_profile_t lpcm = {
  .id = "LPCM",
  .mime = NULL,
  .label = LPCM_LABEL
};

static dlna_profile_t *
probe_lpcm (AVFormatContext *ctx)
{
  AVCodecContext *codec;
  dlna_profile_t *p;
  char mime[128];
  
  /* check for valid file extension */
  if (!match_file_extension (ctx->filename, LPCM_KNOWN_EXTENSIONS))
    return NULL;

  codec = audio_profile_get_codec (ctx);
  if (!codec)
    return NULL;

  /* check for 16-bit signed network-endian PCM codec  */
  if (codec->codec_id != CODEC_ID_PCM_S16BE &&
      codec->codec_id != CODEC_ID_PCM_S16LE)
    return NULL;

  /* supported channels: mono or stereo */
  if (codec->channels > 2)
    return NULL;

  /* supported sampling rate: 44.1 and 48 kHz */
  if (codec->sample_rate != 44100 && codec->sample_rate != 48000)
    return NULL;

  p = set_profile (&lpcm);
  sprintf (mime, "%s;rate=%d;channels=%d",
           LPCM_MIME_TYPE, codec->sample_rate, codec->channels);
  p->mime = strdup (mime);
  
  return p;
}

dlna_registered_profile_t dlna_profile_audio_lpcm = {
  .id = DLNA_PROFILE_AUDIO_LPCM,
  .probe = probe_lpcm,
  .next = NULL
};
