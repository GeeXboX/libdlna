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

/* Summary of VMW9 Profiles
 * Simple and Main profiles are identified by FourCC WMV3
 * Advanced profile is identified by FourCC WVC1
 *
 * Profile 	 Level 	 Max. Bitrate 	 Format
 *
 * Simple 	 Low 	 96 Kbps 	 176 x 144 @ 15 Hz (QCIF)
 *               Medium  384 Kbps 	 240 x 176 @ 30 Hz
 *                                       352 x 288 @ 15 Hz (CIF)
 *
 * Main 	 Low 	 2 Mbps 	 320 x 240 @ 24 Hz (QVGA)
 *               Medium	 10 Mbps 	 720 x 480 @ 30 Hz (480p)
 *                                       720 x 576 @ 25 Hz (576p)
 *               High 	 20 Mbps 	 1920 x 1080 @ 30 Hz (1080p)
 *
 * Advanced 	 L0 	 2 Mbps  	 352 x 288 @ 30 Hz (CIF)
 *	         L1 	 10 Mbps 	 720 x 480 @ 30 Hz (NTSC-SD)
 *                                       720 x 576 @ 25 Hz (PAL-SD)
 *	         L2 	 20 Mbps 	 720 x 480 @ 60 Hz (480p)
 *                                       1280 x 720 @ 30 Hz (720p)
 *               L3 	 45 Mbps 	 1920 x 1080 @ 24 Hz (1080p)
 *                                       1920 x 1080 @ 30 Hz (1080i)
 *                                       1280 x 720 @ 60 Hz (720p)
 *          	 L4 	 135 Mbps 	 1920 x 1080 @ 60 Hz (1080p)
 *                                       2048 x 1536 @ 24 Hz
 */

#include <stdlib.h>
#include <string.h>

#include "dlna.h"
#include "profiles.h"
#include "containers.h"

typedef enum {
  WMV9_AUDIO_UNKNOWN,
  WMV9_AUDIO_WMA_BASELINE,
  WMV9_AUDIO_WMA_FULL,
  WMV9_AUDIO_WMA_PRO,
  WMV9_AUDIO_MP3
} wmv9_profile_audio_t;

typedef struct wmv9_profile_s {
  int max_width;
  int max_height;
  int fps_num;
  int fps_den;
  int max_bitrate;
} wmv9_profile_t;

static wmv9_profile_t wmv9_profile_simple_low[] = {
  { 176, 144, 15, 1, 96000}
};

static wmv9_profile_t wmv9_profile_simple_medium[] = {
  { 240, 176, 30, 1, 384000},
  { 240, 176, 30000, 1001, 384000},
  { 352, 288, 15, 1, 384000}
};

static wmv9_profile_t wmv9_profile_main_medium[] = {
  { 720, 480, 30, 1, 10000000},
  { 720, 480, 30000, 1001, 10000000},
  { 720, 576, 25, 1, 10000000}
};

static wmv9_profile_t wmv9_profile_main_high[] = {
  { 1920, 1080, 30, 1, 20000000},
  { 1920, 1080, 30000, 1001, 20000000}
};

/* Medium resolution video (Main profile at Medium Level)
   with baseline WMA audio */
static dlna_profile_t wmvmed_base = {
  .id = "WMVMED_BASE",
  .mime = MIME_VIDEO_WMV,
  .label = LABEL_VIDEO_SD
};

/* Medium resolution video (Main profile at Medium Level)
   with full WMA audio */
static dlna_profile_t wmvmed_full = {
  .id = "WMVMED_FULL",
  .mime = MIME_VIDEO_WMV,
  .label = LABEL_VIDEO_SD
};

/* Medium resolution video (Main profile at Medium Level)
   with WMA professional audio */
static dlna_profile_t wmvmed_pro = {
  .id = "WMVMED_PRO",
  .mime = MIME_VIDEO_WMV,
  .label = LABEL_VIDEO_HD
};

/* High resolution video (Main profile at High Level)
   with full WMA audio */
static dlna_profile_t wmvhigh_full = {
  .id = "WMVHIGH_FULL",
  .mime = MIME_VIDEO_WMV,
  .label = LABEL_VIDEO_HD
};

/* High resolution video (Main profile at High Level)
   with WMA professional audio */
static dlna_profile_t wmvhigh_pro = {
  .id = "WMVHIGH_PRO",
  .mime = MIME_VIDEO_WMV,
  .label = LABEL_VIDEO_HD
};

/* HighMAT profile */
static dlna_profile_t wmvhm_base __attribute__ ((unused)) = {
  .id = "WMVHM_BASE",
  .mime = MIME_VIDEO_WMV,
  .label = LABEL_VIDEO_SD
};

/* Low resolution video (Simple Profile at Low Level)
   with baseline WMA audio */
static dlna_profile_t wmvspll_base = {
  .id = "WMVSPLL_BASE",
  .mime = MIME_VIDEO_WMV,
  .label = LABEL_VIDEO_QCIF15
};

/* Low resolution video (Simple Profile at Medium Level)
   with baseline WMA audio */
static dlna_profile_t wmvspml_base = {
  .id = "WMVSPML_BASE",
  .mime = MIME_VIDEO_WMV,
  .label = LABEL_VIDEO_CIF15
};

/* Low resolution video (Simple Profile at Medium Level) with MP3 audio */
static dlna_profile_t wmvspml_mp3 = {
  .id = "WMVSPML_MP3",
  .mime = MIME_VIDEO_WMV,
  .label = LABEL_VIDEO_CIF15
};

static int
is_valid_wmv9_video_profile (wmv9_profile_t profile[], int size,
                             AVStream *vs, AVCodecContext *vc)
{
  int i;

  for (i = 0; i < size / sizeof (wmv9_profile_t); i++)
    if (vc->width <= profile[i].max_width &&
        vc->height <= profile[i].max_height &&
        vs->r_frame_rate.num == profile[i].fps_num &&
        vs->r_frame_rate.den == profile[i].fps_den &&
        vc->bit_rate <= profile[i].max_bitrate)
      return 1;

  /* video properties do not fit the requested profile */
  return 0;
}

static wmv9_profile_audio_t
wmv9_get_audio_profile (AVCodecContext *ac)
{
  /* check for WMA codec */
  if (ac->codec_id == CODEC_ID_WMAV1 || ac->codec_id == CODEC_ID_WMAV2)
  {
    if (ac->sample_rate <= 48000)
    {
      if (ac->bit_rate <= 193000)
      {
        /* WMA Baseline: stereo, up to 48 KHz, up to 192,999 bps */
        if (ac->channels > 2)
          return WMV9_AUDIO_UNKNOWN;

        return WMV9_AUDIO_WMA_BASELINE;
      }
      else if (ac->bit_rate <= 385000)
      {
        /* WMA Full: stereo, up to 48 KHz, up to 385 Kbps */
        if (ac->channels > 2)
          return WMV9_AUDIO_UNKNOWN;
        
        return WMV9_AUDIO_WMA_FULL;
      }
    
      /* bitrate > 385 Kbps */
      return WMV9_AUDIO_UNKNOWN;
    }
    else if (ac->sample_rate <= 96000)
    {
      /* WMA Professional: up to 7.1 channels, up to 1.5 Mbps and 96 KHz */
      if (ac->channels > 8)
        return WMV9_AUDIO_UNKNOWN;

      if (ac->bit_rate > 1500000)
        return WMV9_AUDIO_UNKNOWN;

      return WMV9_AUDIO_WMA_PRO;
    }

    /* sampling rate > 96 Kbps */
    return WMV9_AUDIO_UNKNOWN;
  }
  else if (ac->codec_id != CODEC_ID_MP3) /* check for MP3 codec */
  {
    /* only mono and stereo are supported */
    if (ac->channels > 2)
      return WMV9_AUDIO_UNKNOWN;

    if (ac->sample_rate != 32000 &&
        ac->sample_rate != 44100 &&
        ac->sample_rate != 48000)
      return WMV9_AUDIO_UNKNOWN;
    
    if (ac->bit_rate > 128000)
      return WMV9_AUDIO_UNKNOWN;

    return WMV9_AUDIO_MP3;
  }
  
  return WMV9_AUDIO_UNKNOWN;
}

static dlna_profile_t *
probe_wmv9 (AVFormatContext *ctx)
{
  av_codecs_t *codecs;
  wmv9_profile_audio_t ap;
  
  /* need to be in ASF container only */
  if (stream_get_container (ctx) != CT_ASF)
    return NULL;

  codecs = av_profile_get_codecs (ctx);
  if (!codecs)
    goto probe_wmv9_end;

  /* check for WMV3 (Simple and Main profiles) video codec */
  if (codecs->vc->codec_id != CODEC_ID_WMV3)
    goto probe_wmv9_end;

  /* get audio profile */
  ap = wmv9_get_audio_profile (codecs->ac);

  /* we'll determine profile and level according to bitrate */
  if (is_valid_wmv9_video_profile (wmv9_profile_simple_low,
                                   sizeof (wmv9_profile_simple_low),
                                   codecs->vs, codecs->vc))
  {
    if (ap == WMV9_AUDIO_WMA_BASELINE)
      return set_profile (&wmvspll_base);

    goto probe_wmv9_end;
  }
  else if (is_valid_wmv9_video_profile (wmv9_profile_simple_medium,
                                        sizeof (wmv9_profile_simple_medium),
                                        codecs->vs, codecs->vc))
  {
    if (ap == WMV9_AUDIO_WMA_BASELINE)
      return set_profile (&wmvspml_base);
    else if (ap == WMV9_AUDIO_MP3)
      return set_profile (&wmvspml_mp3);

    goto probe_wmv9_end;
  }
  else if (is_valid_wmv9_video_profile (wmv9_profile_main_medium,
                                        sizeof (wmv9_profile_main_medium),
                                        codecs->vs, codecs->vc))
  {
    if (ap == WMV9_AUDIO_WMA_BASELINE)
      return set_profile (&wmvmed_base);
    else if (ap == WMV9_AUDIO_WMA_FULL)
      return set_profile (&wmvmed_full);
    else if (ap == WMV9_AUDIO_WMA_PRO)
      return set_profile (&wmvmed_pro);

    goto probe_wmv9_end;
  }
  else if (is_valid_wmv9_video_profile (wmv9_profile_main_high,
                                        sizeof (wmv9_profile_main_high),
                                        codecs->vs, codecs->vc))
  {
    if (ap == WMV9_AUDIO_WMA_FULL)
      return set_profile (&wmvhigh_full);
    else if (ap == WMV9_AUDIO_WMA_PRO)
      return set_profile (&wmvhigh_pro);

    goto probe_wmv9_end;
  }

 probe_wmv9_end:
  if (codecs)
    free (codecs);
  return NULL;
}

dlna_registered_profile_t dlna_profile_av_wmv9 = {
  .id = DLNA_PROFILE_AV_WMV9,
  .extensions = "asf,wmv",
  .probe = probe_wmv9,
  .next = NULL
};
