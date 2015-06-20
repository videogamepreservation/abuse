#include "timing.hpp"
#include "loader2.hpp"
#include "chars.hpp"
#include "specs.hpp"
#include "parse.hpp"
#include "lisp.hpp"
#include "jrand.hpp"
#include "menu.hpp"
#include "dev.hpp"
#include "director.hpp"

#include "dev.hpp"
#include "light.hpp"
#include "morph.hpp"
#include <math.h>
#include "dprint.hpp"
#include "clisp.hpp"
#include "compiled.hpp"
#include "sbar.hpp"
#include "help.hpp"
#include "loadgame.hpp"
#include "nfserver.hpp"
#include "specache.hpp"

extern int past_startup;

property_manager *prop;
int *backtiles;
int *foretiles;
JCFont *big_font,*console_font;
int nforetiles,nbacktiles,f_wid,f_hi,b_wid,b_hi,total_songs=0,sfx_volume,music_volume,sound_avail=0;
song *current_song=NULL;

ushort current_start_type,start_position_type,last_start_number;
int light_buttons[13];
int joy_picts[2*9];
palette *pal;

int big_font_pict=-1,small_font_pict=-1,console_font_pict=-1,cdc_logo;

int title_screen;

color_filter *color_table;


int border_tile,window_texture,
    raise_volume,lower_volume,record_button,play_button,music_button,sfx_button,
    window_colors,pause_image,damage_pict,block_pict,vmm_image,earth,earth_mask,clouds,
    numbers[10],ok_button,cancel_button;

int start_running=0;

int c_mouse1,c_mouse2,c_normal,c_target;

long bg_xmul,bg_xdiv,bg_ymul,bg_ydiv;    // brackground scroll rates
char mouse_scrolling=0,palettes_locked=1,view_shift_disabled=0;

int light_connection_color;


image *load_image(spec_entry *e, bFILE *fp)
{
  image *im=new image(e,fp);
  if (scale_mult!=1 || scale_div!=1)
    im->resize(im->width()*scale_mult/scale_div,im->height()*scale_mult/scale_div);
  return im;  
}

image *load_image(bFILE *fp)
{
  image *im=new image(fp);
  if (scale_mult!=1 || scale_div!=1)
    im->resize(im->width()*scale_mult/scale_div,im->height()*scale_mult/scale_div);

  return im;  
}

void use_file(char *filename, bFILE *&fp, spec_directory *&sd)
{
  char fn[100];
  fp=open_file(filename,"rb"); 
  if (fp->open_failure()) 
  { 
    delete fp;
    sprintf(fn,"art/%s",filename);  
    fp=open_file(fn,"rb");
    if (fp->open_failure()) 
    { 
      dprintf("Unable to open file %s\n",filename); 
      delete fp;
      exit(1); 
    } 
  } 
  sd=new spec_directory(fp);
}

void done_file(bFILE *&fp, spec_directory *&sd) 
{
  delete fp;
  delete sd;
}

void insert_tiles(char *filename)
{
  bFILE *fp=open_file(filename,"rb");
  if (!fp->open_failure())
  {
    int ft=0,bt=0;
    spec_directory sd(fp);
    delete fp;
    int i=0;
    for (;i<sd.total;i++)    
    {
      spec_entry *se=sd.entries[i];
      if (se->type==SPEC_FORETILE)
        ft++;
      else if (se->type==SPEC_BACKTILE)
        bt++;      
    }
    if (bt)
      dprintf("%s : adding %d background tiles (range %d-%d)\n",
	     filename,bt,nbacktiles,nbacktiles+bt-1);
    if (ft)
      dprintf("%s : adding %d foreground tiles (range %d-%d)\n",
	     filename,ft,nforetiles,nforetiles+bt-1);
    if (!ft && !bt)    
      dprintf("Warning : file %s has no background or foreground tiles\n");
    else
    {
      int fon=nforetiles,bon=nbacktiles;
      if (ft)
        foretiles=(int *)jrealloc(foretiles,sizeof(int)*(nforetiles+ft),"foretile id array ptrs");
      if (bt)
        backtiles=(int *)jrealloc(backtiles,sizeof(int)*(nbacktiles+bt),"foretile id array ptrs");

      for (i=0;i<sd.total;i++)
      {
	if (sd.entries[i]->type==SPEC_FORETILE)
	{
	  foretiles[fon]=cash.reg(filename,sd.entries[i]->name);
	  fon++;
	  nforetiles++;
	}
	if (sd.entries[i]->type==SPEC_BACKTILE)
	{
	  backtiles[bon]=cash.reg(filename,sd.entries[i]->name);
	  bon++;
	  nbacktiles++;
	}
      }    
    }
  } else
    dprintf("Warning : insert_tiles -> file %s could not be read from\n",filename);
}

void load_tiles(Cell *file_list)
{  
  bFILE *fp;
  spec_directory *sd;
  spec_entry *spe;
  
  
  int num,i; 



  Cell *fl;
  int old_fsize=nforetiles,
      old_bsize=nbacktiles;

  for (fl=file_list;!NILP(fl);fl=lcdr(fl))
  {
    fp=open_file(lstring_value(lcar(fl)),"rb");
    if (fp->open_failure()) 
    {
      dprintf("Warning : open %s for reading\n",lstring_value(lcar(fl)));
      delete fp;
    }
    else
    {
      sd=new spec_directory(fp); 
      delete fp;
      int i;
      for (i=0;i<sd->total;i++)
      {
	spe=sd->entries[i];
        switch (spe->type)
        {
          case SPEC_BACKTILE : 
            if (!sscanf(spe->name,"%d",&num))
              dprintf("Warning : background tile %s has no number\n",spe->name);
            else if (nbacktiles<=num) nbacktiles=num+1;
          break;

          case SPEC_FORETILE : 
            if (!sscanf(spe->name,"%d",&num))
              dprintf("Warning : foreground tile %s has no number\n",spe->name);
            else if (nforetiles<=num) nforetiles=num+1;
          break;
        }
      } 
      delete sd;
    }
  }

  // enlarge the arrays if needed.
  if (nbacktiles>old_bsize)
  {
    backtiles=(int *)jrealloc(backtiles,sizeof(int)*nbacktiles,"backtile id array ptrs");
    memset(backtiles+old_bsize,-1,(nbacktiles-old_bsize)*sizeof(int));
  }

  if (nforetiles>old_fsize)
  {
    foretiles=(int *)jrealloc(foretiles,sizeof(int)*nforetiles,"foretile id array ptrs");
    memset(foretiles+old_fsize,-1,(nforetiles-old_fsize)*sizeof(int));
  }


// now load them up
  for (fl=file_list;!NILP(fl);fl=lcdr(fl))
  {
    char *fn=lstring_value(lcar(fl));
    fp=open_file(fn,"rb");
    if (!fp->open_failure()) 
    {
      sd=new spec_directory(fp); 
      delete fp;

      int i;
      for (i=0;i<sd->total;i++)
      {
	spe=sd->entries[i];
        switch (spe->type)
        {
          case SPEC_BACKTILE : 
	    
            if (sscanf(spe->name,"%d",&num))
	    {
	      if (backtiles[num]>=0)
	      {
		dprintf("Warning : background tile %d redefined\n",num);
		cash.unreg(backtiles[num]);
	      }
	      backtiles[num]=cash.reg(fn,spe->name,SPEC_BACKTILE);	      
	    }
            break;
          case SPEC_FORETILE : 
            if (sscanf(spe->name,"%d",&num))
	    {
	      if (foretiles[num]>=0)
	      {
		dprintf("Warning : foreground tile %d redefined\n",num);
		cash.unreg(foretiles[num]);
	      }
	      foretiles[num]=cash.reg(fn,spe->name,SPEC_FORETILE);
	    }
            break;
        }
      } 
      delete sd;
    } else delete fp;
  }

}


extern unsigned char fnt6x13[192*104];
char lsf[256]="abuse.lsp";

void load_data(int argc, char **argv)
{
  bFILE *fp;
  spec_directory *sd;
  total_objects=0;
  total_weapons=0;
  weapon_types=NULL;

  figures=NULL;
  nforetiles=nbacktiles=0;
  foretiles=NULL;
  backtiles=NULL;
  pal=NULL;
  color_table=NULL;

  int should_save_sd_cache=0;
  
  if (0)
//  if (!net_start())
  {
    bFILE *load=open_file("sd_cache.tmp","rb");
    if (!load->open_failure())
      sd_cache.load(load);
    else should_save_sd_cache=1;
    delete load;
  }



  if (!net_start())              // don't let them specify a startup file we are connect elsewhere
  {
    for (int i=1;i<argc;i++)
    {
      if (!strcmp(argv[i],"-lsf"))
      {
	i++;
	strcpy(lsf,argv[i]);
      }
      if (!strcmp(argv[i],"-a"))
      {
	i++;
	sprintf(lsf,"addon/%s/%s.lsp",argv[i],argv[i]);      
      }
    }
  } else if (!get_remote_lsf(net_server,lsf))
  {
    dprintf("Unable to get remote lsf from %s\n",net_server);
    exit(0);
  }
  char prog[100],*cs;

  c_mouse1=cash.reg("art/dev.spe","c_mouse1",SPEC_IMAGE,0);
  c_mouse2=cash.reg("art/dev.spe","c_mouse2",SPEC_IMAGE,0);
  c_normal=cash.reg("art/dev.spe","c_normal",SPEC_IMAGE,0);
  c_target=cash.reg("art/dev.spe","c_target",SPEC_IMAGE,0);


  sprintf(prog,"(load \"%s\")\n",lsf);

  cs=prog;
  if (!eval(compile(cs)))
  {
    dprintf("unable to open file '%s'\n",lsf);
    exit(0);
  }
  compiled_init();
  clear_tmp();
  resize_tmp(0x4000);

  dprintf("engine : registering base graphics\n");
  for (int z=0;z<=11;z++)
  {
    char nm[10];
    sprintf(nm,"l%d",z);
    light_buttons[z]=cash.reg("art/dev.spe",nm,SPEC_IMAGE,0);
  }


  image *tmp_image=new image(192,104,fnt6x13);
  big_font=new JCFont(tmp_image);
  delete tmp_image;


  char *ff;
  if (DEFINEDP(symbol_value(make_find_symbol("frame_file"))))
    ff=lstring_value(symbol_value(make_find_symbol("frame_file")));
  else
    ff="art/frame.spe";

  ok_button   =      cash.reg(ff,"dev_ok",SPEC_IMAGE);
  cancel_button  =   cash.reg(ff,"cancel",SPEC_IMAGE);

//  clouds      =      cash.reg(ff,"clouds",SPEC_IMAGE);

  lower_volume=      cash.reg(ff,"lower_volume",SPEC_IMAGE);
  raise_volume=      cash.reg(ff,"raise_volume",SPEC_IMAGE);
  music_button=      cash.reg(ff,"music",SPEC_IMAGE);
  sfx_button=        cash.reg(ff,"sound_fx",SPEC_IMAGE);
  record_button=     cash.reg(ff,"record",SPEC_IMAGE);  
  play_button=       cash.reg(ff,"play",SPEC_IMAGE);
  window_colors=     cash.reg(ff,"window_colors",SPEC_IMAGE);
  pause_image=       cash.reg(ff,"pause_image",SPEC_IMAGE);
  vmm_image=         cash.reg(ff,"vmm",SPEC_IMAGE);
  border_tile=       cash.reg(ff,"border_tile",SPEC_IMAGE);
  window_texture=    cash.reg(ff,"window_texture",SPEC_IMAGE);
  

  help_screens=NULL;
  total_help_screens=0;  

  if (DEFINEDP(symbol_value(l_help_screens)))
  {
    void *v=symbol_value(l_help_screens);
    char *ff=lstring_value(CAR(v));  v=CDR(v);
    total_help_screens=0;
    while (v) { total_help_screens++; v=CDR(v); }
    if (total_help_screens)
    {
      help_screens=(int *)jmalloc(sizeof(int)*total_help_screens,"help image ids");      
      v=CDR(symbol_value(l_help_screens));
      int i=0;
      for (;v;v=CDR(v),i++)
        help_screens[i]=cash.reg(ff,lstring_value(CAR(v)),SPEC_IMAGE);      
    } 
    else
      dprintf("Warning no help images following filename\n");
  }  
     
  int j,b,i;
  for (i=1;i<argc;i++)
  {
    if (!strcmp(argv[i],"-ec"))
      set_symbol_value(l_empty_cache,true_symbol);
    if (!strcmp(argv[i],"-t"))
    {
      i++;
      insert_tiles(argv[i]);
    }
  }

  if (DEFINEDP(symbol_value(l_title_screen)))
    title_screen=cash.reg_object(NULL,symbol_value(l_title_screen),SPEC_IMAGE,1);
  else title_screen=-1;

  if (DEFINEDP(symbol_value(l_cdc_logo)))
    cdc_logo=cash.reg_object(NULL,symbol_value(l_cdc_logo),SPEC_IMAGE,1);
  else cdc_logo=-1;
  
  start_position_type=0xffff;
  for(i=0;i<total_objects;i++)
    if (!strcmp(object_names[i],"START"))
      start_position_type=i;
  if (start_position_type==0xffff)
  {
    dprintf("No object named START, cannot start game.\n");
    exit(0);
  }

  sbar.load();
  
  load_number_icons();


  ERROR(nbacktiles,"No background tiles defined!");
  ERROR(nforetiles,"No foreground tiles defined!");
  ERROR(foretiles[0]>=0,"No black (0) foreground tile defined!");
  ERROR(backtiles[0]>=0,"No black (0) background tile defined!");
  ERROR(big_font_pict!=-1 || small_font_pict!=-1,
	"No font loaded (use load_big_font or load_small_font)!");
  f_wid=cash.foret(foretiles[0])->im->width();
  f_hi=cash.foret(foretiles[0])->im->height();
  b_wid=cash.backt(backtiles[0])->im->width();
  b_hi=cash.backt(backtiles[0])->im->height();

  if (should_save_sd_cache)
  {
    bFILE *save=open_file("sd_cache.tmp","wb");
    if (!save->open_failure())
      sd_cache.save(save);
    delete save;
  }
      
  sd_cache.clear();


  past_startup=1;


}      





char *load_script(char *name)
{
  char fn[100];
  char *s;
  
  sprintf(fn,"%s",name);
  bFILE *fp=open_file(fn,"rb");
  if (fp->open_failure())
  {
    delete fp;
    return NULL;
  }
  
  long l=fp->file_size();
  s=(char *)jmalloc(l+1,"loaded script");
  ERROR(s,"Malloc error in load_script");
  
  fp->read(s,l);  
  s[l]=0;
  delete fp;
  return s;  
}
















