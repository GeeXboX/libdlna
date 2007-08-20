#include <stdlib.h>
#include <stdio.h>

#include "dlna.h"

int
main (int argc, char **argv)
{
  dlna_profile_t *p;
  dlna_org_flags_t flags;

  flags = DLNA_ORG_FLAG_STREAMING_TRANSFER_MODE |
    DLNA_ORG_FLAG_BACKGROUND_TRANSFERT_MODE |
    DLNA_ORG_FLAG_CONNECTION_STALL |
    DLNA_ORG_FLAG_DLNA_V15;

  if (argc < 2)
  {
    printf ("usage: %s media_filename\n", argv[0]);
    return -1;
  }

  dlna_init ();
  dlna_register_all_media_profiles ();
  
  p = dlna_guess_media_profile (argv[1]);
  if (p)
  {
    char *protocol_info;
    
    printf ("Found demuxer\n");
    printf ("ID: %s\n", p->id);
    printf ("MIME: %s\n", p->mime);
    printf ("Label: %s\n", p->label);

    protocol_info = dlna_write_protocol_info (DLNA_PROTOCOL_INFO_TYPE_HTTP,
                                              DLNA_ORG_PLAY_SPEED_NORMAL,
                                              DLNA_ORG_CONVERSION_NONE,
                                              DLNA_ORG_OPERATION_RANGE,
                                              flags, p);
    printf ("Protocol Info: %s\n", protocol_info);
    free (protocol_info);
  }
  else
    printf ("Unknown format\n");
  
  return 0;
}
