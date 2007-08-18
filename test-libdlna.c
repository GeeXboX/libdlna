#include <stdlib.h>
#include <stdio.h>

#include "dlna.h"

int
main (int argc, char **argv)
{
  dlna_profile_t *p;
  
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
    printf ("Found demuxer\n");
    printf ("ID: %s\n", p->id);
    printf ("MIME: %s\n", p->mime);
    printf ("Label: %s\n", p->label);
  }
  else
    printf ("Unknown format\n");
  
  return 0;
}
