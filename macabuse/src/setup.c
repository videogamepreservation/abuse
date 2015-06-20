#ifdef __WATCOMC__

#include "specs.hpp"
#include "id.hpp"
#include "jwindow.hpp"
#include "sound.hpp"
#include "guistat.hpp"
#include "timing.hpp"
#include "video.hpp"
#include "input.hpp"
#include "scroller.hpp"
#include "lisp.hpp"
#include "lisp_gc.hpp"
#include "dprint.hpp"
#include "cache.hpp"


extern int sosEZGetConfig(char *szName);

extern window_manager *eh;
extern palette *lastl;

extern unsigned char fnt6x13[192*104];

/*int card_ids[] =
{  _ADLIB_GOLD_8_MONO,
   _ADLIB_GOLD_8_MONO,
   _SOUNDSCAPE_8_MONO,
   _ESS_AUDIODRIVE_8_MONO,
   _GUS_8_MONO,
   _GUS_MAX_8_MONO,
   _MICROSOFT_8_MONO,
   _MV_PAS_8_MONO,
   _MV_PAS_8_MONO,
   _RAP10_8_MONO,
   _SOUND_BLASTER_8_MONO,          // SB
   _SOUND_BLASTER_8_MONO,          // SB pro
   _SB16_8_MONO,
   _SB16_8_MONO,
   _SOUND_MASTER_II_8_MONO,
   _SOUND_BLASTER_8_MONO,
   _SOUND_BLASTER_8_MONO };
*/



int get_option(char *name);

JCFont *cfg_font;

static  int highest_help=0;


extern int jmalloc_max_size;

FILE *open_FILE(char *filename, char *mode);
void setup(int argc, char **argv)
{

  char tmp_name[200];
  if (get_filename_prefix())
    sprintf(tmp_name,"%s%s",get_filename_prefix(),"sndcard.cfg");
  else strcpy(tmp_name,"sndcard.cfg");

  FILE *fp=fopen(tmp_name,"rb");
  
  if (!fp)
  {
    fp=fopen("setup.exe","rb");
    if (!fp)
    {
      dprintf("Sound not configured and no setup program, cannot play sound\n");
      return ;
    }
    fclose(fp);
    system("setup.exe");
  } else fclose(fp);
    
  sosEZGetConfig(tmp_name);

}
  
  
#else

void setup(int argc, char **argv)
{ ; } 


#endif
