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

#define MPEG4_KNOWN_EXTENSIONS "aac,3gp,mp4,mov,qt,m4a"

/* Profile for audio media class content */
static dlna_profile_t aac_adts = {
  .id = "AAC_ADTS",
  .mime = MIME_AUDIO_ADTS,
  .label = LABEL_AUDIO_2CH
};

/* Profile for audio media class content */
static dlna_profile_t aac_adts_320 = {
  .id = "AAC_ADTS_320",
  .mime = MIME_AUDIO_ADTS,
  .label = LABEL_AUDIO_2CH
};

/* Profile for audio media class content */
static dlna_profile_t aac_iso = {
  .id = "AAC_ISO",
  .mime = MIME_AUDIO_MPEG_4,
  .label = LABEL_AUDIO_2CH
};

/* Profile for audio media class content */
static dlna_profile_t aac_iso_320 = {
  .id = "AAC_ISO_320",
  .mime = MIME_AUDIO_MPEG_4,
  .label = LABEL_AUDIO_2CH
};

/* Profile for audio media class content. In the case of AAC LTP profiles,
   both the ISO file formats and the ADTS format are supported by
   the same profile. */
static dlna_profile_t aac_ltp_iso __attribute__ ((unused)) = {
  .id = "AAC_LTP_ISO",
  .mime = NULL,
  .label = LABEL_AUDIO_2CH
};

/* Profile for audio media class content with up to 5.1 channels */
static dlna_profile_t aac_ltp_mult5_iso __attribute__ ((unused)) = {
  .id = "AAC_LTP_MULT5_ISO",
  .mime = NULL,
  .label = LABEL_AUDIO_MULTI
};

/* Profile for audio media class content with up to 7.1 channels */
static dlna_profile_t aac_ltp_mult7_iso __attribute__ ((unused)) = {
  .id = "AAC_LTP_MULT7_ISO",
  .mime = NULL,
  .label = LABEL_AUDIO_MULTI
};

/* Profile for audio media class content with up to 5.1 channels */
static dlna_profile_t aac_mult5_adts = {
  .id = "AAC_MULT5_ADTS",
  .mime = MIME_AUDIO_ADTS,
  .label = LABEL_AUDIO_MULTI
};

/* Profile for audio media class content with up to 5.1 channels */
static dlna_profile_t aac_mult5_iso = {
  .id = "AAC_MULT5_ISO",
  .mime = MIME_AUDIO_MPEG_4,
  .label = LABEL_AUDIO_MULTI
};

/* Profile for audio media class content */
static dlna_profile_t heaac_l2_adts __attribute__ ((unused)) = {
  .id = "HEAAC_L2_ADTS",
  .mime = MIME_AUDIO_ADTS,
  .label = LABEL_AUDIO_2CH
};

/* Profile for audio media class content */
static dlna_profile_t heaac_l2_iso __attribute__ ((unused)) = {
  .id = "HEAAC_L2_ISO",
  .mime = NULL,
  .label = LABEL_AUDIO_2CH
};

/* Profile for audio media class content */
static dlna_profile_t heaac_l3_adts __attribute__ ((unused)) = {
  .id = "HEAAC_L3_ADTS",
  .mime = MIME_AUDIO_ADTS,
  .label = LABEL_AUDIO_2CH
};

/* Profile for audio media class content */
static dlna_profile_t heaac_l3_iso __attribute__ ((unused)) = {
  .id = "HEAAC_L3_ISO",
  .mime = NULL,
  .label = LABEL_AUDIO_2CH
};

/* Profile for audio media class content with up to 5.1 channels */
static dlna_profile_t heaac_mult5_adts __attribute__ ((unused)) = {
  .id = "HEAAC_MULT5_ADTS",
  .mime = MIME_AUDIO_ADTS,
  .label = LABEL_AUDIO_MULTI
};

/* Profile for audio media class content with up to 5.1 channels */
static dlna_profile_t heaac_mult5_iso __attribute__ ((unused)) = {
  .id = "HEAAC_MULT5_ISO",
  .mime = NULL,
  .label = LABEL_AUDIO_MULTI
};

/* Profile for audio media class content */
static dlna_profile_t heaac_l2_adts_320 __attribute__ ((unused)) = {
  .id = "HEAAC_L2_ADTS_320",
  .mime = MIME_AUDIO_ADTS,
  .label = LABEL_AUDIO_2CH
};

/* Profile for audio media class content */
static dlna_profile_t heaac_l2_iso_320 __attribute__ ((unused)) = {
  .id = "HEAAC_L2_ISO_320",
  .mime = NULL,
  .label = LABEL_AUDIO_2CH
};

/* Profile for audio media class content */
static dlna_profile_t bsac_iso __attribute__ ((unused)) = {
  .id = "BSAC_ISO",
  .mime = NULL,
  .label = LABEL_AUDIO_2CH
};

/* Profile for audio media class content with up to 5.1 channels */
static dlna_profile_t bsac_mult5_iso __attribute__ ((unused)) = {
  .id = "BSAC_MULT5_ISO",
  .mime = NULL,
  .label = LABEL_AUDIO_MULTI
};

static int
audio_is_valid_aac (AVCodecContext *ac)
{
  if (!ac)
    return 0;
  
  /* TODO: need to check for HE-AAC, LTP and BSAC */
  if (ac->codec_id != CODEC_ID_AAC)
    return 0;
  
  /* supported sampling rate:
     8, 11.025, 12, 16, 22.05, 24, 32, 44.1 and 48 kHz */
  if (ac->sample_rate != 8000 &&
      ac->sample_rate != 11025 &&
      ac->sample_rate != 12000 &&
      ac->sample_rate != 16000 &&
      ac->sample_rate != 22050 &&
      ac->sample_rate != 24000 &&
      ac->sample_rate != 32000 &&
      ac->sample_rate != 44100 &&
      ac->sample_rate != 48000)
    return 0;

  return 1;
}

int
audio_is_valid_aac_stereo (AVCodecContext *ac)
{
  if (!ac)
    return 0;

  if (!audio_is_valid_aac (ac))
    return 0;
  
  if (ac->channels > 2)
    return 0;
  
  if (ac->bit_rate > 576000)
    return 0;
    
  return 1;
}

int
audio_is_valid_aac_mult5 (AVCodecContext *ac)
{
  if (!ac)
    return 0;

  if (!audio_is_valid_aac (ac))
    return 0;
  
  if (ac->channels != 5)
    return 0;
  
  if (ac->bit_rate > 1444000)
    return 0;
    
  return 1;
}

static dlna_profile_t *
probe_mpeg4 (AVFormatContext *ctx)
{
  AVCodecContext *codec = NULL;
  int adts = 0;
  
  /* check for valid file extension */
  if (!match_file_extension (ctx->filename, MPEG4_KNOWN_EXTENSIONS))
    return NULL;

  if (!strcasecmp (get_file_extension (ctx->filename), "aac"))
    adts = 1;

  codec = audio_profile_get_codec (ctx);
  if (!codec)
    return NULL;
  
  /* check for AAC codec */
  /* TODO: need to check for HE-AAC, LTP and BSAC */
  if (codec->codec_id == CODEC_ID_AAC)
  {
    if (audio_is_valid_aac_mult5 (codec))
      return adts ?
        set_profile (&aac_mult5_adts): set_profile (&aac_mult5_iso);
    else if (audio_is_valid_aac_stereo (codec))
      return adts ?
        set_profile (&aac_adts_320) : set_profile (&aac_iso_320);
    else
      return adts ?
        set_profile (&aac_adts) : set_profile (&aac_iso);
  }

  return NULL;
}

dlna_registered_profile_t dlna_profile_audio_mpeg4 = {
  .id = DLNA_PROFILE_AUDIO_MPEG4,
  .probe = probe_mpeg4,
  .next = NULL
};
