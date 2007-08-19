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

#define MP3_KNOWN_EXTENSIONS "mp3"
#define MP3_MIME_TYPE "audio/mpeg"
#define MP3_LABEL "2-ch"

/* Profile for audio media class content */
static dlna_profile_t mp3 = {
  .id = "MP3",
  .mime = MP3_MIME_TYPE,
  .label = MP3_LABEL
};

/* Profile for audio media class content with extensions
   for lower sampling rates and bitrates */
static dlna_profile_t mp3x = {
  .id = "MP3X",
  .mime = MP3_MIME_TYPE,
  .label = MP3_LABEL
};

/* Audio encoding must be MPEG-1 Layer 3 */
static dlna_profile_t *
probe_mp3 (AVFormatContext *ctx)
{
  AVStream *stream;
  AVCodecContext *codec;
  
  /* check for valid file extension */
  if (!match_file_extension (ctx->filename, MP3_KNOWN_EXTENSIONS))
    return NULL;

  /* should only have 1 stream */
  if (ctx->nb_streams > 1)
    return NULL;

  stream = ctx->streams[0];
  codec = stream->codec;
  
  /* which obviously should be an audio one */
  if (codec->codec_type != CODEC_TYPE_AUDIO)
    return NULL;

  /* check for MP3 codec */
  if (codec->codec_id != CODEC_ID_MP3)
    return NULL;
  
  /* only mono and stereo are supported */
  if (codec->channels > 2)
    return NULL;

  switch (codec->sample_rate)
  {
  case 16000:
  case 22050:
  case 24000:
    switch (codec->bit_rate)
    {
    case 8000:
    case 16000:
    case 24000:
    case 32000:
    case 40000:
    case 48000:
    case 56000:
    case 64000:
    case 80000:
    case 96000:
    case 112000:
    case 128000:
    case 160000:
    case 192000:
    case 224000:
    case 256000:
    case 320000:
      return set_profile (&mp3x);
    default:
      return NULL;
    }
    break;
  case 32000:
  case 44100:
  case 48000:
    switch (codec->bit_rate)
    {
    case 32000:
    case 40000:
    case 48000:
    case 56000:
    case 64000:
    case 80000:
    case 96000:
    case 112000:
    case 128000:
    case 160000:
    case 192000:
    case 224000:
    case 256000:
    case 320000:
      return set_profile (&mp3);
    default:
      return NULL;
    }
    break;
  default:
    return NULL;
  }
  
  return NULL;
}

dlna_registered_profile_t dlna_profile_audio_mp3 = {
  .id = DLNA_PROFILE_AUDIO_MP3,
  .probe = probe_mp3,
  .next = NULL
};
