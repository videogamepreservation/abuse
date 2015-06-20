#include "fonts.hpp"
#include <ctype.h>

texture_font::texture_font(image *letters, image *font_pattern)
{ fntpat=font_pattern;
  let=letters;
  tl=(let->width()+1)/32;
  th=(let->height()+1)/8;
}

void texture_font::put_char(image *screen,  int x, int y, char ch)
{ if (fntpat)
    fntpat->put_part_masked(screen,let, x,y,
       ((int)ch%32)*tl,((int)ch/32)*th,0,0,tl-1,th-1);
  else let->put_part(screen,x,y,((int)ch%32)*tl,((int)ch/32)*th,
     ((int)ch%32)*tl+tl-1,((int)ch/32)*th+th-1,1);
}

void texture_font::put_string(image *screen, int x, int y, char *st)
{ while (*st)
  { put_char(screen,x,y,*st);
    st++;
    x+=tl;
  }
}


void JCFont::put_string(image *screen, int x, int y, char *st, int color)
{ while (*st)
  { put_char(screen,x,y,*st,color);
    st++;
    x+=tl;
  }
}


void JCFont::put_char(image *screen,  int x, int y, char ch, int color)
{
  if (let[ch])
  {
    if (color>=0)
      let[ch]->put_color(screen,x,y,color);
    else let[ch]->put_image(screen,x,y);
  }
}

JCFont::JCFont(image *letters)
{
  tl=(letters->width()+1)/32;
  th=(letters->height()+1)/8;    

  image tmp(tl,th);
  
  int ch;
  
  for (ch=0;ch<256;ch++)
  {    
    tmp.clear();    
    letters->put_part(&tmp,0,0,((int)ch%32)*tl,((int)ch/32)*th,
		      ((int)ch%32)*tl+tl-1,((int)ch/32)*th+th-1,1);  
    let[ch]=new trans_image(&tmp,"JCfont");
  } 
}

JCFont::~JCFont()
{
  int ch;
  for (ch=0;ch<256;ch++)  
    delete let[ch];  
}

