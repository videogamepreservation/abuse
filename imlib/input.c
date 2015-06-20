#include "input.hpp"
#include "macs.hpp"


void button::remap(filter *f)
{
  if (visual)
  {
    f->apply(visual);
    if (pressed)
      f->apply(pressed);
  }
}

void button_box::press_button(int id)      // if button box doesn't contain id, nothing happens
{
}

void button_box::remap(filter *f)
{
  for (button *b=buttons;b;b=(button *)b->next)
    b->remap(f);
}

ifield *button_box::find(int search_id)
{
  if (search_id==id) return this;
  for (ifield *i=(ifield *)buttons;i;i=i->next)
    if (search_id==i->id) return i;
  return NULL;
}

button_box::button_box(int X, int Y, int ID, int MaxDown, button *Buttons, ifield *Next)
{
  x=X; y=Y; id=ID; next=Next; 
  buttons=Buttons;
  maxdown=MaxDown;
  if (buttons && maxdown) buttons->push();  // the first button is automatically selected!
}

button_box::~button_box()
{
  while (buttons)
  {
    button *b=buttons;
    buttons=(button *)buttons->next;
    delete b;
  }
}

void button_box::area(int &x1, int &y1, int &x2, int &y2, window_manager *wm)
{
  button *b=buttons;
  if (!b) return ;
  else
  {
    b->area(x1,y1,x2,y2,wm);
    int xp1,yp1,xp2,yp2;
    for (b=(button *)b->next;b;b=(button *)b->next)
    {
      b->area(xp1,yp1,xp2,yp2,wm);
      if (xp1<x1) x1=xp1;
      if (xp2>x2) x2=xp2;
      if (yp1<y1) y1=yp1;
      if (yp2>y2) y2=yp2;
    }          
  }
}

void button_box::draw_first(image *screen, window_manager *wm)
{
  for (button *b=buttons;b;b=(button *)b->next)
    b->draw_first(screen,wm);
}

void button_box::draw(int active, image *screen, window_manager *wm)
{
  return ;
}

char *button_box::read()
{
  for (button *b=buttons;b;b=(button *)b->next)
  {
    if (*((int *)b->read())==0)
      return (char *)b;
  }
  return NULL;
}

void button_box::handle_event(event &ev, image *screen, window_manager *wm, input_manager *im)
{
  switch (ev.type)
  {
    case EV_MOUSE_BUTTON :
    {
      int x1,y1,x2,y2;
      int found=0;
      for (button *b=buttons;!found && b;b=(button *)b->next)  // see if the user clicked on a button
      {
	b->area(x1,y1,x2,y2,wm);
	if (ev.mouse_move.x>=x1 && ev.mouse_move.x<=x2 && 
	    ev.mouse_move.y>=y1 && ev.mouse_move.y<=y2)
	{
	  b->handle_event(ev,screen,wm,im);

	  int total=0;
	  button *b2=buttons;
	  for (;b2;b2=(button *)b2->next)
	    if (*((int *)b2->read())==0)
	      total++;

	  int draw=1;
	  if (*((int *)b->read())==0)  // did the user press or release the button
	  {
	    if (total>maxdown)
	    {
	      for (b2=buttons;total>maxdown && b2;b2=(button *)b2->next)
	        if ((b!=b2 || maxdown==0) && *((int *)b2->read())==0)
		{
		  total--;
		  b2->push();
		  b2->draw_first(screen,wm);
		}
	    }
	    b->draw_first(screen,wm);
	  } else if (total==0 && maxdown)
	    b->push();    // don't let the user de-press a button if non others are selected.     

	  found=1; // don't look at anymore buttons

	}
      }
    } break;   
  }
}


void button_box::add_button(button *b)
{
  b->next=buttons;
  buttons=b;
}


void button_box::arrange_left_right(window_manager *wm)
{
  button *b=buttons;
  int x_on=x,x1,y1,x2,y2;
  for (;b;b=(button *)b->next)
  {
    b->area(x1,y1,x2,y2,wm);
    b->x=x_on;
    b->y=y;
    x_on+=(x2-x1+1)+1;
  }  
}

void button_box::arrange_up_down(window_manager *wm)
{  
  button *b=buttons;
  int y_on=y,x1,y1,x2,y2;
  for (;b;b=(button *)b->next)
  {
    b->area(x1,y1,x2,y2,wm);
    b->y=y_on;
    b->x=x;
    y_on+=(y2-y1+1)+1;
  }  
}

void button::change_visual(image *new_visual)
{
  CHECK(visual);
  visual=new_visual;
}

void button::area(int &x1, int &y1, int &x2, int &y2, window_manager *wm)
{  
  x1=x; y1=y; 
  if (pressed)
  {
    x2=x+pressed->width()-1;
    y2=y+pressed->height()-1;
  }
  else
  {
    if (text)
    {    
      x2=x+wm->font()->width()*strlen(text)+6; 
      y2=y+wm->font()->height()+6; 
    } else
    {
      x2=x+6+visual->width();
      y2=y+6+visual->height();
    }
  }
}


button::button(int X, int Y, int ID, char *Text, ifield *Next)
{  
  x=X; y=Y; id=ID; 
  act_id=-1;
  text=strcpy((char *)jmalloc(strlen(Text)+1,"input button"),Text); 
  up=1; next=Next; act=0;
  visual=NULL;
  pressed=NULL;
}


button::button(int X, int Y, int ID, image *vis, ifield *Next)
{ x=X; y=Y; id=ID; text=NULL; 
  act_id=-1;
  visual=vis; up=1; next=Next; act=0; 
  pressed=NULL;
}

button::button(int X, int Y, int ID, image *Depressed, image *Pressed, image *active, ifield *Next)
{ x=X; y=Y; id=ID; text=NULL; 
  act_id=-1;
  visual=Depressed; up=1; next=Next; act=0; 
  pressed=Pressed;
  act_pict=active;
}


void text_field::change_data(char *new_data, int new_cursor, // cursor==-1, does not change it.
			     int active, image *screen, window_manager *wm)
{
  if (strlen(format)<strlen(new_data))
    data=(char *)jrealloc(data,strlen(new_data),"text field input");

  strcpy(data,new_data);
  if (new_cursor!=-1)
    cur=new_cursor;
  draw_first(screen,wm);
  draw(active,screen,wm);
}

char *text_field::read()
{
  while (*data && data[strlen(data)-1]==' ') data[strlen(data)-1]=0;
  return data;
}

#ifdef __POWERPC__
#pragma global_optimizer on
#endif

void text_field::handle_event(event &ev, image *screen, window_manager *wm, input_manager *im)
{
  int xx;
  if (ev.type==EV_KEY)
  {
    switch (ev.key)
    {
      case JK_LEFT : if (cur) { draw_cur(wm->dark_color(),screen,wm); cur--;
                           draw_cur(wm->bright_color(),screen,wm); } break; 
      case JK_RIGHT : if (cur<strlen(format)-1) { draw_cur(wm->dark_color(),screen,wm); cur++;
                           draw_cur(wm->bright_color(),screen,wm); } break; 
      case JK_END : if (cur!=last_spot()) 
                          { draw_cur(wm->dark_color(),screen,wm); cur=last_spot(); 
                            if (cur==strlen(format)-1) cur--; 
                           draw_cur(wm->bright_color(),screen,wm); } break; 
      case JK_HOME : if (cur) 
                          { draw_cur(wm->dark_color(),screen,wm); cur=0;
                           draw_cur(wm->bright_color(),screen,wm); } break; 
      case JK_BACKSPACE : if (cur)
         { draw_cur(wm->dark_color(),screen,wm); cur--;
           for (xx=cur;xx<strlen(format)-1;xx++)
             data[xx]=data[xx+1];
           data[strlen(format)-1]=' ';
           draw_text(screen,wm);
           draw_cur(wm->bright_color(),screen,wm); 
           wm->push_event(new event(id,(char *)this));
         } break; 
      default : if (ev.key>=' ' && ev.key<='~')
         { 
           draw_cur(wm->dark_color(),screen,wm); 
           for (xx=strlen(format)-1;xx>cur && xx>0;xx--)
             data[xx]=data[xx-1];
           data[cur]=ev.key;
           if (cur<strlen(format)-1)
             cur++;
	   data[strlen(format)]=0;
           draw_text(screen,wm);
           draw_cur(wm->bright_color(),screen,wm); 
           wm->push_event(new event(id,(char *)this));
         } break;
    }
  } 
}

#ifdef __POWERPC__
#pragma global_optimizer reset
#endif

void text_field::draw(int active, image *screen, window_manager *wm)
{
  if (active)
  {
    screen->rectangle(xstart(wm),y,xend(wm),yend(wm),wm->bright_color());
    draw_cur(wm->bright_color(),screen,wm);
  }
  else
  {
    screen->rectangle(xstart(wm),y,xend(wm),yend(wm),wm->dark_color());
    draw_cur(wm->dark_color(),screen,wm);
  }
}

void text_field::area(int &x1, int &y1, int &x2, int &y2, window_manager *wm)
{
  x1=x; y1=y; 
  x2=xend(wm);
  y2=yend(wm);
}

text_field::text_field(int X, int Y, int ID, char *Prompt, char *Format, 
                                                     char *Data, ifield *Next)
{
  int slen=(strlen(Format)>strlen(Data) ? strlen(Format) : strlen(Data));

  x=X; y=Y; id=ID;
  prompt=strcpy((char *)jmalloc(strlen(Prompt)+1,"text_field::prompt"),Prompt);
  format=strcpy((char *)jmalloc(slen+1,"text_field::format"),Format);
  data=strcpy((char *)jmalloc(slen+1,"text_field::data"),Data);
  cur=strlen(data);
  while (cur && data[cur-1]==' ') cur--;
  next=Next;
}

text_field::text_field(int X, int Y, int ID, char *Prompt, char *Format, 
                               double Data, ifield *Next)
{
  char num[20];
  sprintf(num,"%lg",Data);  
  int slen=(strlen(Format)>strlen(num) ? strlen(Format) : strlen(num));
  x=X; y=Y; id=ID;
  prompt=strcpy((char *)jmalloc(strlen(Prompt)+1,"text_field::prompt"),Prompt);
  format=strcpy((char *)jmalloc(slen+1,"text_field::format"),Format);
  data=strcpy((char *)jmalloc(slen+1,"text_field::data"),num);
  cur=strlen(num);
  while (cur && data[cur-1]==' ') cur--;
  next=Next;
}


void button::push()
{ up=!up; }

void button::handle_event(event &ev, image *screen, window_manager *wm, input_manager *im)
{
  if ((ev.type==EV_KEY && ev.key==13) || (ev.type==EV_MOUSE_BUTTON &&
                                         ev.mouse_button))
  {
    int  x1,y1,x2,y2;
    area(x1,y1,x2,y2,wm);
    up=!up;
    draw_first(screen,wm);
    draw(act,screen,wm);
    wm->push_event(new event(id,(char *)this));
  }
}

void button::draw(int active, image *screen, window_manager *wm)
{
  int x1,y1,x2,y2,color=(active ? wm->bright_color() : wm->medium_color());  
  area(x1,y1,x2,y2,wm); 
  if (active!=act  && act_id!=-1 && active)
    wm->push_event(new event(act_id,NULL));
    
  if (pressed)
  {
    if (up)
    {
      if (!active)
        visual->put_image(screen,x,y);
      else
        pressed->put_image(screen,x,y);
    } else act_pict->put_image(screen,x,y);
  }
  else
  {
    screen->rectangle(x1+2,y1+2,x2-2,y2-2,color);
    act=active;
  }
}

void button::draw_first(image *screen, window_manager *wm)
{
  if (pressed)  
    draw(0,screen,wm);
  else
  {

    int x1,y1,x2,y2;
    area(x1,y1,x2,y2,wm);
    

    if (up)
    {
      screen->rectangle(x1,y1,x2,y2,wm->black());
//      screen->wiget_bar(,wm->bright_color(),wm->medium_color(),wm->dark_color()); 
      screen->wiget_bar(x1+1,y1+1,x2-1,y2-1,wm->bright_color(),wm->medium_color(),wm->dark_color()); 
      if (text)
      {
        wm->font()->put_string(screen,x+4,y+5,text,wm->black());
        wm->font()->put_string(screen,x+3,y+4,text);
      }
      else visual->put_image(screen,x+3,y+3,1);
    } else
    {
      screen->line(x1,y1,x2,y1,wm->dark_color());
      screen->line(x1,y1,x1,y2,wm->dark_color());
      screen->line(x2,y1+1,x2,y2,wm->bright_color());
      screen->line(x1+1,y2,x2,y2,wm->bright_color());
      screen->bar(x1+1,y1+1,x2-1,y2-1,wm->medium_color());
      if (visual)
        visual->put_image(screen,x1+3,y1+3,1);
      else
      {
        wm->font()->put_string(screen,x+4,y+5,text,wm->black());
        wm->font()->put_string(screen,x+3,y+4,text);
      }
    }  
  }
}

void text_field::draw_first(image *screen, window_manager *wm)
{
  wm->font()->put_string(screen,x,y+3,prompt);
  screen->bar(xstart(wm),y,xend(wm),yend(wm),wm->dark_color());
  wm->font()->put_string(screen,xstart(wm)+1,y+3,data);
}


void text_field::draw_cur(int color, image *screen, window_manager *wm)
{
  screen->bar(xstart(wm)+cur*wm->font()->width()+1,
                      yend(wm)-2,
                      xstart(wm)+(cur+1)*wm->font()->width(),
                      yend(wm)-1,color);
}



info_field::info_field(int X, int Y, int ID, char *info, ifield *Next)
{
  x=X; y=Y; id=ID; next=Next;
  text=strcpy((char *)jmalloc(strlen(info)+1,"info_field"),info);
  w=-1;
}


void info_field::area(int &x1, int &y1, int &x2, int &y2, window_manager *wm)
{
  if (w==-1)     // if we haven't calculated this yet
  {
    int fw=wm->font()->width(),fh=wm->font()->height(),maxw=0;
    char *info=text;
    for (w=fw,h=fh+1;*info;info++)
    {
      if (w>maxw) maxw=w;
      if (*info=='\n')
      {
	h+=fh+1;
	w=1;
      }
      else w+=fw;      
    }
    w=maxw;
  }      
  x1=x;
  y1=y;
  x2=x+w;
  y2=y+h;
}

void info_field::put_para(image *screen, char *st, int dx, int dy, 
			  int xspace, int yspace, JCFont *font, int color)
{
  int ox=dx;
  while (*st)
  {
    if (*st=='\n')
    {
      dx=ox;
      dy+=yspace;
    }
    else
    {
      font->put_char(screen,dx,dy,*st,color);
      dx+=xspace;
    }
    st++;
  }
}

void info_field::draw_first(image *screen, window_manager *wm)
{
  put_para(screen,text,x+1,y+1,wm->font()->width(),wm->font()->height(),wm->font(),wm->black());
  put_para(screen,text,x,y,wm->font()->width(),wm->font()->height(),wm->font(),wm->bright_color());
}




