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


/*  short *port_list,*irq_list,*dma_list;
  get_card_info(0,port_list,irq_list,dma_list);

  jmalloc_init(0x150000);         // make sure user has enough memory to run game
  jmalloc_uninit(); 

  jmalloc_max_size=500000;        // use a small amount of memory so SOS has some space to work with
  jmalloc_init(400000); 
  lisp_init(0xf000,0xf000);

  char *cfg_command="(load \"snd_cfg.lsp\")";
  char *cs=cfg_command;
  if (!eval(compile(cs)) || get_option("-setup"))
  {
    int i;
    for (i=1;i<argc;i++)
      if (!strcmp(argv[i],"-nosound"))
      {
	sound_init(argc,argv);  // let the sound system know there will be no sound
	dprintf("not runnign setup because -nosound option is on\n");
	return ;                // go no further with sound
      }

    char *cfg_start_command="(load \"lisp/config.lsp\")";
    char *cs=cfg_start_command;
    if (!eval(compile(cs)))
    {
      dprintf("Missing lisp/config.lsp, make sure you unzipped with -d\n"
	      "skipping sound configuration\n");
      char *s[2];  s[0]=argv[0]; s[1]="-nosound";
      sound_init(1,s);                                 // initialize with -nosound as only command-line arg
      return;
    }



    stat_man=new text_status_manager();
    image_init();

    palette pal;  // create a usable palette, we can't load one because the file might not be there,
                  // we are not connect to a server yet.

    memset(pal.addr(),0,768);
    for (i=0;i<64;i++)
      pal.set(i,i*3,i*3,i*3);
    for (i=64;i<128;i++)
      pal.set(i,(i-64)*3,0,0);

    pal.set(32,200,190,240);
    set_mode(0x13,argc,argv);
    pal.load();


    JCFont fnt(new image(192,104,fnt6x13));
    eh=new window_manager(screen,&pal,50,40,20,
			  &fnt);  

    timer_init();


    int x=0;
    int card=-1;    // none
    int irq=-1;
    int dma=-1;
    int port=-1;
    
    cfg_font=eh->font();
    for (;!get_option("-nosound") && x==0;)
    {
      x=l_menu(symbol_value(make_find_symbol("cfg_main_menu")));
      if (x==0)    // select card
      {
	int c=l_menu(symbol_value(make_find_symbol("card_menu")));
	if (c==0)
	  card=0;
	else if (c>0)
	{
	  card=c-1;
	  int sos_card_id=card_ids[c];
	  short *port_list,*irq_list,*dma_list;
	  get_card_info(sos_card_id,port_list,irq_list,dma_list);
	  
	  int p=l_menu(make_menu("port_title","port_prefix",port_list));
	  if (p>0)
	  {
	    port=port_list[p];
	    int i=l_menu(make_menu("irq_title","irq_prefix",irq_list));
	    if (i>0)
	    {
	      irq=irq_list[i];
	      int d=l_menu(make_menu("dma_title","dma_prefix",dma_list));
	      if (d>0)
	        dma=dma_list[d];
	    }
	  }
	  jfree(port_list);
	  jfree(irq_list);
	  jfree(dma_list);

	}
      }
    }
    if (get_option("-nosound"))
      card=irq=port=dma=-1;
    else if (dma!=-1)
    {
      FILE *fp=fopen("snd_cfg.lsp","wb");
      if (!fp)
        dprintf("Unable to open snd_cfg.lsp for writing\n"
		"cannot save sound configuration\n");
      else
      {
	fprintf(fp,
		"(setq sound_card_type %d)\n"
		"(setq sound_card_port %d)\n"
		"(setq sound_card_irq %d)\n"
		"(setq sound_card_dma %d)\n",
		card,port,irq,port,dma);
      }
    }
    
    delete eh;

    close_graphics();
    timer_uninit();
    image_uninit();
    delete stat_man;
    delete lastl;

    cash.empty();
    cash.create_lcache();   
  } else
  {
        if (DEFINEDP(symbol_value(make_find_symbol("sound_card_type"))))



  }
 
  lisp_uninit();
  l_user_stack.clean_up();
  l_ptr_stack.clean_up();

  crc_man.clean_up();
  mem_report("end.mem");
  jmalloc_uninit();

  jmalloc_max_size=4000000; */
}
  
  
#else

void setup(int argc, char **argv)
{ ; } 


#endif
