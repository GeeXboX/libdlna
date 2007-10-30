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

/* Profile for audio media class content */
static dlna_profile_t ac3 = {
  .id = "AC3",
  .mime = MIME_AUDIO_DOLBY_DIGITAL,
  .label = LABEL_AUDIO_2CH_MULTI
};

int
audio_is_valid_ac3 (AVCodecContext *ac)
{
  if (!ac)
    return 0;

  /* check for AC3 codec */
  if (ac->codec_id != CODEC_ID_AC3)
    return 0;
  
  /* supported channels: 1/0, 2/0, 3/0, 2/1, 3/1, 2/2, 3/2 */
  if (ac->channels > 5)
    return 0;

  /* supported sampling rate: 32, 44.1 and 48 kHz */
  if (ac->sample_rate != 32000 &&
      ac->sample_rate != 44100 &&
      ac->sample_rate != 48000)
    return 0;

  /* supported bitrate: 32 Kbps - 640 Kbps */
  if (ac->bit_rate < 32000 || ac->bit_rate > 640000)
    return 0;

  return 1;
}

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
  
  if (audio_is_valid_ac3 (codec))
    return set_profile (&ac3);
  return NULL;
}

dlna_registered_profile_t dlna_profile_audio_ac3 = {
  .id = DLNA_PROFILE_AUDIO_AC3,
  .probe = probe_ac3,
  .next = NULL
};
