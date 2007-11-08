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

#ifndef _DLNA_H_
#define _DLNA_H_

#define DLNA_STRINGIFY(s)         DLNA_TOSTRING(s)
#define DLNA_TOSTRING(s) #s

#define LIBDLNA_VERSION_INT  ((0<<16)+(1<<8)+3)
#define LIBDLNA_VERSION      0.1.3
#define LIBDLNA_BUILD        LIBDLNA_VERSION_INT

#define LIBDLNA_IDENT        "DLNA " DLNA_STRINGIFY(LIBDLNA_VERSION)

typedef enum {
  DLNA_PROTOCOL_INFO_TYPE_UNKNOWN,
  DLNA_PROTOCOL_INFO_TYPE_HTTP,
  DLNA_PROTOCOL_INFO_TYPE_RTP,
  DLNA_PROTOCOL_INFO_TYPE_ANY
} dlna_protocol_info_type_t;

/* DLNA.ORG_PS: play speed parameter (integer)
 *     0 invalid play speed
 *     1 normal play speed
 */
typedef enum {
  DLNA_ORG_PLAY_SPEED_INVALID = 0,
  DLNA_ORG_PLAY_SPEED_NORMAL = 1,
} dlna_org_play_speed_t;

/* DLNA.ORG_CI: conversion indicator parameter (integer)
 *     0 not transcoded
 *     1 transcoded
 */
typedef enum {
  DLNA_ORG_CONVERSION_NONE = 0,
  DLNA_ORG_CONVERSION_TRANSCODED = 1,
} dlna_org_conversion_t;

/* DLNA.ORG_OP: operations parameter (string)
 *     "00" (or "0") neither time seek range nor range supported
 *     "01" range supported
 *     "10" time seek range supported
 *     "11" both time seek range and range supported
 */
typedef enum {
  DLNA_ORG_OPERATION_NONE                  = 0x00,
  DLNA_ORG_OPERATION_RANGE                 = 0x01,
  DLNA_ORG_OPERATION_TIMESEEK              = 0x10,
} dlna_org_operation_t;

/* DLNA.ORG_FLAGS, padded with 24 trailing 0s
 *     80000000  31  senderPaced
 *     40000000  30  lsopTimeBasedSeekSupported
 *     20000000  29  lsopByteBasedSeekSupported
 *     10000000  28  playcontainerSupported
 *      8000000  27  s0IncreasingSupported
 *      4000000  26  sNIncreasingSupported
 *      2000000  25  rtspPauseSupported
 *      1000000  24  streamingTransferModeSupported
 *       800000  23  interactiveTransferModeSupported
 *       400000  22  backgroundTransferModeSupported
 *       200000  21  connectionStallingSupported
 *       100000  20  dlnaVersion15Supported
 *
 *     Example: (1 << 24) | (1 << 22) | (1 << 21) | (1 << 20)
 *       DLNA.ORG_FLAGS=01700000[000000000000000000000000] // [] show padding
 */
typedef enum {
  DLNA_ORG_FLAG_SENDER_PACED               = (1 << 31),
  DLNA_ORG_FLAG_TIME_BASED_SEEK            = (1 << 30),
  DLNA_ORG_FLAG_BYTE_BASED_SEEK            = (1 << 29),
  DLNA_ORG_FLAG_PLAY_CONTAINER             = (1 << 28),
  DLNA_ORG_FLAG_S0_INCREASE                = (1 << 27),
  DLNA_ORG_FLAG_SN_INCREASE                = (1 << 26),
  DLNA_ORG_FLAG_RTSP_PAUSE                 = (1 << 25),
  DLNA_ORG_FLAG_STREAMING_TRANSFER_MODE    = (1 << 24),
  DLNA_ORG_FLAG_INTERACTIVE_TRANSFERT_MODE = (1 << 23),
  DLNA_ORG_FLAG_BACKGROUND_TRANSFERT_MODE  = (1 << 22),
  DLNA_ORG_FLAG_CONNECTION_STALL           = (1 << 21),
  DLNA_ORG_FLAG_DLNA_V15                   = (1 << 20),
} dlna_org_flags_t;

typedef enum {
  DLNA_CLASS_IMAGE,
  DLNA_CLASS_AUDIO,
  DLNA_CLASS_AV,
  DLAN_CLASS_COLLECTION
} dlna_media_class_t;

typedef enum {
  /* Image Class */
  DLNA_PROFILE_IMAGE_JPEG,
  DLNA_PROFILE_IMAGE_PNG,
  /* Audio Class */
  DLNA_PROFILE_AUDIO_AC3,
  DLNA_PROFILE_AUDIO_AMR,
  DLNA_PROFILE_AUDIO_ATRAC3,
  DLNA_PROFILE_AUDIO_LPCM,
  DLNA_PROFILE_AUDIO_MP3,
  DLNA_PROFILE_AUDIO_MPEG4,
  DLNA_PROFILE_AUDIO_WMA,
  /* AV Class */
  DLNA_PROFILE_AV_MPEG1,
  DLNA_PROFILE_AV_MPEG2,
  DLNA_PROFILE_AV_MPEG4_PART2,
  DLNA_PROFILE_AV_MPEG4_PART10, /* a.k.a. MPEG-4 AVC */
  DLNA_PROFILE_AV_WMV9
} dlna_media_profile_t;

typedef struct dlna_profile_s {
  const char *id;
  const char *mime;
  const char *label;
} dlna_profile_t;

void dlna_init (void);
void dlna_register_all_media_profiles (void);
void dlna_register_media_profile (dlna_media_profile_t profile);

dlna_profile_t *dlna_guess_media_profile (const char *filename);
char * dlna_write_protocol_info (dlna_protocol_info_type_t type,
                                 dlna_org_play_speed_t speed,
                                 dlna_org_conversion_t ci,
                                 dlna_org_operation_t op,
                                 dlna_org_flags_t flags,
                                 dlna_profile_t *p);

#endif /* _DLNA_H_ */
