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

#define AC3_KNOWN_EXTENSIONS "ac3"
#define AC3_MIME_TYPE "audio/vnd.dolby.dd-raw"
#define AC3_LABEL "2-ch multi"

/* Profile for audio media class content */
static dlna_profile_t ac3 = {
  .id = "AC3",
  .mime = AC3_MIME_TYPE,
  .label = AC3_LABEL
};

static dlna_profile_t *
probe_ac3 (AVFormatContext *ctx)
{
  AVCodecContext *codec;
  
  /* check for valid file extension */
  if (!match_file_extension (ctx->filename, AC3_KNOWN_EXTENSIONS))
    return NULL;

  codec = audio_profile_get_codec (ctx);
  if (!codec)
    return NULL;
  
  /* which obviously should be an audio one */
  if (codec->codec_type != CODEC_TYPE_AUDIO)
    return NULL;

  /* check for AC3 codec */
  if (codec->codec_id != CODEC_ID_AC3)
    return NULL;

  /* supported channels: 1/0, 2/0, 3/0, 2/1, 3/1, 2/2, 3/2 */
  if (codec->channels > 5)
    return NULL;

  /* supported sampling rate: 32, 44.1 and 48 kHz */
  if (codec->sample_rate != 32000 &&
      codec->sample_rate != 44100 &&
      codec->sample_rate != 48000)
    return NULL;

  /* supported bitrate: 64 Kbps - 640 Kbps */
  if (codec->bit_rate < 64000 || codec->bit_rate > 640000)
    return NULL;

  return set_profile (&ac3);
}

dlna_registered_profile_t dlna_profile_audio_ac3 = {
  .id = DLNA_PROFILE_AUDIO_AC3,
  .probe = probe_ac3,
  .next = NULL
};
