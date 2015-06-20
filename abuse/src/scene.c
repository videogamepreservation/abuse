#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include "specs.hpp"
#include "timage.hpp"
#include "jwindow.hpp"
#include "fonts.hpp"
#include "timing.hpp"
#include "scene.hpp"
#include "game.hpp"
#include "parse.hpp"
#include "cache.hpp"
#include <fcntl.h>


class string_node
{
  string_node *l,*r;
  char *n;
public :  
  string_node(char *Name, string_node *Left=NULL, string_node *Right=NULL)
  { l=Left;
    r=Right;
    n=strcpy((char *)jmalloc(strlen(Name)+1,"string node"),Name);
  }
  ~string_node() { jfree(n); }               
  char *name() { return n; }
  string_node *left() { return l; }
  string_node *right() { return r; }    		           
} ;


/*class string_tree
{
  string_node *root;
public :
  string_tree() { root=NULL; }
  void insert(char *name);
     
} ; */



char scene_filename[100];

class scene_frame
{
public :
  int picture;
  long time,xc,yc;  

  scene_frame *next;  
  scene_frame(char *&s);
  ~scene_frame() { cash.unreg(picture); }
} ;


scene_frame::scene_frame(char *&s)
{
  char tmp_name[50];
  expect(get_token(s,tmp_name),sWORD,s);

  picture=cash.reg(scene_filename,tmp_name);
  if (picture<0) 
  {
    printf("Frame image not found (%s)\n",tmp_name);
    exit(0);
  }

  
  xc=yc=0;
  time=-1;  
  if (token_type(s)==sLEFT_PAREN)  // is a descriptor included?
  {
    next_token(s);
    
    xc=get_number(s);
    if (token_type(s)!=sRIGHT_PAREN)
    {      
      yc=get_number(s);
      if (token_type(s)!=sRIGHT_PAREN)
        time=get_number(s);
    }    
    expect(get_token(s,tmp_name),sRIGHT_PAREN,s);    
  }

  next=NULL;  
}



class scene_sequence : public linked_node
{
public :   
  char *n;
  scene_frame *first;
  scene_sequence *next;  
  scene_sequence(char *&s);
  ~scene_sequence();
} ;



scene_sequence::~scene_sequence()
{ jfree(n); 
  while (first) 
  { scene_frame *p=first; 
    first=first->next;
    delete p; 
  }
}

 
scene_sequence::scene_sequence(char *&s)
{
  scene_frame *cur;
  char tmp_name[50];
  expect(token_type(s),sLEFT_PAREN,s);    
  next_token(s);

  expect(get_token(s,tmp_name),sWORD,s);  
  n=strcpy((char *)jmalloc(strlen(tmp_name)+1,"sequence name"),tmp_name);  
  cur=first=new scene_frame(s);  

  while (token_type(s)!=sRIGHT_PAREN)    
  {
    cur->next=new scene_frame(s);
    cur=cur->next;    

    next=NULL;  
  }  
  next_token(s);  
  next=NULL;  
}


class scene_sequence_list
{
public :
  scene_sequence *first;
  scene_sequence_list(char *&s);  
  scene_sequence *get_seq(char *seq_name);
  ~scene_sequence_list();  
} ;

scene_sequence_list::~scene_sequence_list()
{
  scene_sequence *p;
  while (first)
  {
    p=first;
    first=first->next;
    delete p;    
  }
}  

scene_sequence_list::scene_sequence_list(char *&s)
{ 
  scene_sequence *cur;  
  expect(token_type(s),sLEFT_PAREN,s);    
  next_token(s);

  cur=first=new scene_sequence(s);   
  while (token_type(s)!=sRIGHT_PAREN)
  {
    cur->next=new scene_sequence(s);
    cur=cur->next;    
  }    
  next_token(s);  
}

scene_sequence *scene_sequence_list::get_seq(char *seq_name)
{
  scene_sequence *s=first;
  while (s && strcmp(s->n,seq_name)) s=s->next;
  if (!s) 
  {
    printf("No sequence named %s\n",seq_name);
    exit(1);    
  }
  return s;       
}


class scene_character
{
  scene_sequence_list *seq_list;

  game_object *me;
  
public :
  char *n;  
  scene_character *next;
  scene_sequence *current_seq;    
  scene_frame *current_frame;
  
  time_marker *last_frame;
  void draw();  
  void area(int &x1, int &y1, int &x2, int &y2);  
  scene_character(char *&s);  
  void set_seq(char *seq_name) 
  { current_seq=seq_list->get_seq(seq_name); }
  int x() { return the_game->screenx(me->x)-cash.fig(current_frame->picture)->xcfg; } 	     
  int y() { return the_game->screeny(me->y)-cash.fig(current_frame->picture)->forward->height(); }
  int next_frame();  // true if sequence is done
  ~scene_character() { jfree(n); delete seq_list; if (last_frame) delete last_frame; }     
} ;


int scene_character::next_frame()
{
  me->x+=current_frame->xc;
  me->y+=current_frame->yc;

  current_frame=current_frame->next;        // advance the picture

  if (last_frame)                           // save the time stamp, delete old one
    delete last_frame;		
  last_frame=new time_marker;		

  if (!current_frame)                      // end of sequence?
  {		  
    current_frame=current_seq->first;	   // reset and return 1
    return 1;
  } 
  else return 0;
}


void scene_character::area(int &x1, int &y1, int &x2, int &y2)
{
   
  x1=x();
  y1=y();
  y2=x2=0;
  
  scene_frame *p=current_seq->first;
  while (p)
  {    
    if (x()+cash.fig(p->picture)->width()-1>x2)
      x2=x()+cash.fig(p->picture)->width()-1;
    if (y()+cash.fig(p->picture)->height()-1>y2)
      y2=y()+cash.fig(p->picture)->height()-1;
    p=p->next;
  }  
}


void scene_character::draw()
{
  cash.fig(current_frame->picture)->forward->put_image(screen,x(),y());
  
}


scene_character::scene_character(char *&s)
{
  char tmp[100];  
  expect(get_token(s,tmp),sLEFT_PAREN,s);        
  expect(get_token(s,tmp),sWORD,s);              
  n=strcpy((char *)jmalloc(strlen(tmp)+1,"scene character name"),tmp);  
  expect(get_token(s,tmp),sNUMBER,s);    

/*  if (atoi(tmp)==0) */

  me=current_level->main_character();

  
  seq_list=new scene_sequence_list(s);          
  current_seq=seq_list->first;  
  current_frame=current_seq->first;
  expect(get_token(s,tmp),sRIGHT_PAREN,s);          

  last_frame=NULL;
  next=NULL;  
}


class scene_character_list
{
public :
  scene_character *first;
  scene_character_list(char *&s);  
  void inital_states(char *&s);  
  scene_character *get(char *name);   
 ~scene_character_list();  
} ;

scene_character  *scene_character_list::get(char *name)
{
  scene_character *s=first;
  while (s)
  {
    if (!strcmp(s->n,name)) return s;
    s=s->next;
  }
  printf("Character %s not found!\n",name);
  exit(1);  
}


scene_character_list::~scene_character_list()
{
  scene_character *p;
  while (first)
  {
    p=first;
    first=first->next;
    delete p;    
  }
}  

scene_character_list::scene_character_list(char *&s)
{ 
  scene_character *cur;  
  expect(token_type(s),sLEFT_PAREN,s);    
  next_token(s);

  cur=first=new scene_character(s);
    
  while (token_type(s)!=sRIGHT_PAREN)
  {
    cur->next=new scene_character(s);
    cur=cur->next;    
  }    
  next_token(s);  
}


void scene_character_list::inital_states(char *&s)
{
  char ch[50],seq[50];

  do
  {
    expect(get_token(s,ch),sLEFT_PAREN,s);  
    expect(get_token(s,ch),sWORD,s);      
    expect(get_token(s,seq),sWORD,s);      
    scene_character *c=first;
    while (c && strcmp(c->n,ch)) c=c->next;
    if (!c)
    {
      printf("No character named %s, at %s\n",ch,s);
      exit(0);
    } else c->set_seq(seq);       
    expect(get_token(s,ch),sRIGHT_PAREN,s);  
  } while (token_type(s)!=sRIGHT_PAREN);
}

class text_blocker
{
public :  
  int x1,y1,x2,y2;  
  text_blocker *next;
  text_blocker(int X1, int Y1, int X2, int Y2, text_blocker *Next) 
  { x1=X1;
    y1=Y1;
    x2=X2;
    y2=Y2;
    next=Next;    
  }     
} ;


int text_draw(int y, int x1, int y1, int x2, int y2, char *buf, 
	      text_blocker *first, JCFont *font)
{
  short cx1,cy1,cx2,cy2,word_size,word_len;
  screen->get_clip(cx1,cy1,cx2,cy2);
  screen->in_clip(x1,y1,x2,y2);  
  int h=font->height()+2,w=font->width(),x=x1,dist;
  y+=y1;
  char *word_start;

  while (*buf)
  {
    do 
    {  
      if (*buf=='\\' && buf[1]=='n')
      {
	x=x1;
	y+=h*2;
	buf+=2;
      }
      
      // skip space
      if (*buf==' ' || *buf=='\r' || *buf=='\n' || *buf=='\t')
      {          
	x+=w;
	while (*buf==' ' || *buf=='\r' || *buf=='\n' || *buf=='\t')   // skip space until next word
          buf++;
      }

      word_start=buf;
      for (word_len=0,word_start=buf,word_size=0;*buf && *buf!=' ' && *buf!='\r' && *buf!='\n' && 
	   *buf!='\t' && (*buf!='\\' || buf[1]!='n');buf++,word_size+=w,word_len++);
      
      if (word_size<x2-x1) // make sure the word can fit on the screen
      {
	if (word_size+x>x2)    // does word not fit on line?
	{
	  y+=h;                // go to next line
	  x=x1;      
	}
      }


      if (y+h<y1)         // word on screen yet?
	x+=word_size;

    } while (y+h<y1);     // if not on screen yet, fetch next word

/*    dist=100;
    for (n=first;n;n=n->next)      
    {
      if (x<n->x1)
        minx=(n->x1-x);
      else if (x>n->x2)
        minx=(x-n->x2);
      else minx=0;

      if (y<n->y1)
        miny=(n->y1-y);
      else if (y>n->y2)
        miny=(y-n->y2);
      else miny=0;
      
      dist=min(dist,max(minx,miny));
    }
      
      
    if (dist<=8) dist=8;
    else if (dist>31) dist=31;      */

    dist=31;
    if (y-y1<dist)
      if (y-y1<8) dist=8;
      else dist=y-y1;
    if (y2-y<dist)
      if (y2-y<8) dist=8;
      else dist=y2-y;         

    if (y>y2) return 0;

    while (word_len--)
    {
      font->put_char(screen,x+1,y+1,*word_start,0);
      font->put_char(screen,x,y,*word_start,32-dist);
      word_start++;
      x+=w;      
    }

  }

  screen->set_clip(cx1,cy1,cx2,cy2);
  return (y<=y1);  
}


struct scene_data_struct       // define a name for the structure so that we can inspect in gdb
{
  int x1,y1,x2,y2,
      pan_vx,pan_yv,pan_steps,
      frame_speed,scroll_speed,pan_speed;  
} scene_data;



    

void play_scene(char *script, char *filename, JCFont *font)
{
  char *s=script;  
  char token[90];
  text_blocker *text_blockers=NULL;

  char *strng=(char *)jmalloc(MAX_SCROLL_DATA,"tmp token space");  
  strcpy(scene_filename,filename);
  
  int x1,y1,x2,y2,done,pan_xv=0,pan_yv=0,pan_steps=0,
      text_loaded=0,frame_speed=100,scroll_speed=50,pan_speed=60,abort=0,text_step=-2;

  short cx1,cy1,cx2,cy2;  

  the_game->draw(1);

  screen->get_clip(cx1,cy1,cx2,cy2);
  screen->set_clip(the_game->viewx1,the_game->viewy1,
		   the_game->viewx2,the_game->viewy2);
  
  
  

  expect(get_token(s,token),sLEFT_PAREN,s);  
  scene_character_list cl(s);
  int y;

  do
  {
    expect(get_token(s,token),sLEFT_PAREN,s);   // list of transitions start
    // ACTIONS    
    time_marker *last_text_time=NULL;	
    time_marker *last_pan_time=NULL;
    do
    {      
      expect(get_token(s,token),sLEFT_PAREN,s);         
      expect(get_token(s,token),sWORD,s);

      if (!strcmp(token,"pan"))
      {
	pan_xv=get_number(s);
	pan_yv=get_number(s);
        pan_steps=get_number(s);
      }
      else if (!strcmp(token,"states"))
        cl.inital_states(s);
      else if (!strcmp(token,"scroll_speed"))
        scroll_speed=get_number(s);
      else if (!strcmp(token,"pan_speed"))
        pan_speed=get_number(s);
      else if (!strcmp(token,"frame_speed"))
        frame_speed=get_number(s);      
      else if (!strcmp(token,"text_region"))
      {
	x1=the_game->viewx1+get_number(s);
	y1=the_game->viewy1+get_number(s);
	x2=the_game->viewx1+get_number(s);
	y2=the_game->viewy1+get_number(s);	
	y=y2-y1;
      } else if (!strcmp(token,"text_block"))
      {
	int sx1=get_number(s)+the_game->viewx1;
	int sy1=get_number(s)+the_game->viewy1;
	int sx2=get_number(s)+the_game->viewx1;
	int sy2=get_number(s)+the_game->viewy1;
	text_blockers=new text_blocker(sx1,sy1,sx2,sy2,text_blockers);
      } else if (!strcmp(token,"block"))
      {
	int sx1,sy1,sx2,sy2;
	expect(get_token(s,token),sWORD,s);	
	cl.get(token)->area(sx1,sy1,sx2,sy2);
	text_blockers=new text_blocker(sx1,sy1,sx2,sy2,text_blockers);	
      }            
      else if (!strcmp(token,"scroll"))
      {
	expect(get_token(s,strng),sSTRING,s);
	text_loaded=1;
	y=y2-y1;
      } else if (!strcmp(token,"wait"))
      {
	expect(get_token(s,token),sWORD,s);
	printf("waiting for %s\n",token);	
	done=0;




	int old_dev=dev;
	dev=dev&(0xffff-DRAW_PEOPLE_LAYER);	


	do
	{	
	  the_game->draw_map();

	  time_marker cur_time;
	  if (pan_steps)
	  {
	    if (last_pan_time)
	    {
	      if ((int)(cur_time.diff_time(last_pan_time)*1000)>pan_speed)
	      {
		the_game->pan(pan_xv,pan_yv);
		pan_steps--;
		delete last_pan_time;
		if (pan_steps)
 		  last_pan_time=new time_marker;
		else last_pan_time=NULL;
	      }
	    } else last_pan_time=new time_marker;
	  }

	  scene_character *first=cl.first;
	  while (first)
	  {
	    first->draw();
	    
	    if (!first->last_frame)
	      first->last_frame=new time_marker;
	    else
	    {
	      int time=first->current_frame->time,advance=0;
	      if (time>=0)
	      {			    
 	        if ((int)(cur_time.diff_time(first->last_frame)*1000)>time)
		  advance=1;
	      }
	      else
	      {
		if ((int)(cur_time.diff_time(first->last_frame)*1000)>frame_speed)
		  advance=1;		
	      }
	    
	      if (advance)
	      {
	        if (!strcmp(token,first->n))      // is this the character we are waiting on?
		{
	          if (first->next_frame())
		    done=1;
		} else first->next_frame();
	      }
	    }	    
	    first=first->next;	    
	  }
	  if (text_loaded)
	  {
	    text_loaded=(!text_draw(y,x1,y1,x2,y2,strng,text_blockers,font));
	    if (last_text_time)
	    {
	      if ((int)(cur_time.diff_time(last_text_time)*1000)>scroll_speed)
	      {	      
  	        y+=text_step;
	        delete last_text_time;
		if (text_loaded)
		  last_text_time=new time_marker;	    
		else
		  last_text_time=NULL;
	      }
	    } else last_text_time=new time_marker;	    
	  } 

	  
	  
	  if (!strcmp(token,"pan"))	  
	    if (pan_steps<=0) done=1;

	  if (!strcmp(token,"text"))
	    if (!text_loaded) done=1;
	    
	  eh->flush_screen();	  
	  while (eh->event_waiting())
	  {	
	    event ev;	    
	    eh->get_event(ev);
	    if (ev.type==EV_KEY)
	    {
	      switch (ev.key)
	      {
	        case JK_UP :
	        case JK_LEFT : 
		  if (scroll_speed>=20) 
		    scroll_speed-=20; 
		  else text_step--;
		  break;
		case JK_RIGHT :
		case JK_DOWN : 
		  if (text_step<-2)
		    text_step++;
		  else if (scroll_speed<200) scroll_speed+=20; 
		  break;
		case JK_ESC : abort=done=1; break;
		case JK_ENTER : done=1; break;
	      }
	    }
	  }
	  
	  
	} while (!done);
	dev=old_dev;
      }      
            
      expect(get_token(s,token),sRIGHT_PAREN,s);     
      
    } while (!abort && token_type(s)!=sRIGHT_PAREN);

    
    if (!abort)
      next_token(s);

    // free up memory allocated 
    while (text_blockers)
    {
      text_blocker *n=text_blockers->next;      
      delete text_blockers;
      text_blockers=n;      
    }
    if (last_text_time)
      delete last_text_time;
    if (last_pan_time)
      delete last_pan_time;

  } while (!abort && token_type(s)!=sRIGHT_PAREN);  
  if (!abort)
    next_token(s);

  jfree(strng);
  screen->set_clip(cx1,cy1,cx2,cy2);  

  the_game->draw(0);
}



