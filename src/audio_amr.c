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

#define AMR_MIME_TYPE "audio/mp4"
#define THREE_GPP_MIME_TYPE "audio/3gpp"
#define MONO_LABEL "mono"
#define TWO_CH_LABEL "2-ch"

/* Profile for audio media class content */
static dlna_profile_t amr = {
  .id = "AMR_3GPP",
  .mime = AMR_MIME_TYPE,
  .label = MONO_LABEL
};

/* Profile for audio media class content */
static dlna_profile_t three_gpp = {
  .id = "AMR_3GPP",
  .mime = THREE_GPP_MIME_TYPE,
  .label = MONO_LABEL
};

/* Profile for audio media class content */
static dlna_profile_t amr_wbplus = {
  .id = "AMR_WBplus",
  .mime = THREE_GPP_MIME_TYPE,
  .label = TWO_CH_LABEL
};

static dlna_profile_t *
probe_amr (AVFormatContext *ctx)
{
  AVStream *stream;
  AVCodecContext *codec;
  char *ext;
  
  /* check for valid file extension */
  ext = get_file_extension (ctx->filename);
  if (strcasecmp (ext, "amr")
      && strcasecmp (ext, "3gp")
      && strcasecmp (ext, "mp4"))
    return NULL;

  /* should only have 1 stream */
  if (ctx->nb_streams > 1)
    return NULL;

  stream = ctx->streams[0];
  codec = stream->codec;
  
  /* which obviously should be an audio one */
  if (codec->codec_type != CODEC_TYPE_AUDIO)
    return NULL;

  /* check for AMR NB/WB audio codec */
  if (codec->codec_id == CODEC_ID_AMR_NB)
  {
    /* only mono is supported */
    if (codec->channels != 1)
      return NULL;

    /* only supports 8 kHz sampling rate */
    if (codec->sample_rate != 8000)
      return NULL;

    /* valid CBR bitrates: 4.75, 5.15, 5.9, 6.7, 7.4, 7.95, 10.2, 12.2 Kbps */
    switch (codec->bit_rate)
    {
    case 4750:
    case 5150:
    case 5900:
    case 6700:
    case 7400:
    case 7950:
    case 10200:
    case 12200:
      if (!strcasecmp (ext, "3gp"))
        return set_profile (&three_gpp);
      else
        return set_profile (&amr);
    default:
      return NULL;
    }
  }
  else if (codec->codec_id == CODEC_ID_AMR_WB)
  {
    /* valid sampling rates: 8, 16, 24, 32 and 48 kHz */
    if (codec->sample_rate != 8000 &&
        codec->sample_rate != 16000 &&
        codec->sample_rate != 24000 &&
        codec->sample_rate != 32000 &&
        codec->sample_rate != 48000)
      return NULL;

    /* supported bit rates: 5.2 Kbps - 48 Kbps */
    if (codec->bit_rate < 5200 || codec->bit_rate > 48000)
      return NULL;

    /* only mono and stereo are supported */
    if (codec->channels > 2)
      return NULL;

    return set_profile (&amr_wbplus);
  }

  return NULL;
}

dlna_registered_profile_t dlna_profile_audio_amr = {
  .id = DLNA_PROFILE_AUDIO_AMR,
  .probe = probe_amr,
  .next = NULL
};
