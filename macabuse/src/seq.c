#include "seq.hpp"
#include "macs.hpp"
#include "parse.hpp"
#include "lisp.hpp"

int sequence::size()
{
  int t=0;
  for (int i=0;i<total;i++)
  {
    if (cash.loaded(seq[i]))
      t+=cash.fig(seq[i])->size();   
  }
  return t;
}

int sequence::cache_in()
{
  int i;
  for (i=0;i<total;i++)
  {
    cash.note_need(seq[i]);
  }
  return 1;
}

sequence::sequence(char *filename, void *pict_list, void *advance_list)
{
  if (item_type(pict_list)==L_STRING)
    total=1;
  else
    total=list_length(pict_list);

  seq=(int *) jmalloc(sizeof(int)*total,"sequence ptr array");
  if (item_type(pict_list)==L_STRING)
    seq[0]=cash.reg_object(filename,pict_list,SPEC_CHARACTER2,1);
  else
  {
    int i;
    for (i=0;i<total;i++) 
    {
      seq[i]=cash.reg_object(filename,lcar(pict_list),SPEC_CHARACTER2,1);
      pict_list=lcdr(pict_list);
    }
  }
}

sequence::~sequence()
{ 
  jfree(seq); 
}

/*sequence::sequence(char *filename, char *picts)
{
  char t[100],*s=picts,imname[100];  
  int i,j;
  total=0;  

  // first count the images
  while (token_type(s)!=sRIGHT_PAREN)
  {
    if (token_type(s)==sLEFT_PAREN)
    {
      get_token(s,t);    // left paren
      get_token(s,t);    // seq
      get_token(s,t);    // name
      i=get_number(s);      
      total+=get_number(s)-i+1;      
      get_token(s,t);    // right paren
    }  
    else 
    { get_token(s,t);
      total++;    
    }    
  }
  

  s=picts;  
  seq=(int *) jmalloc(sizeof(int)*total,"sequence ptr array");
  
  for (i=0;i<total;)
  {
    if (get_token(s,t)==sLEFT_PAREN)
    {
      get_token(s,t);      // left paren
      if (strcmp(t,"seq"))
      {
	dprintf("Expected seq at %s\n",s);
	exit(0);	
      }
      get_token(s,t);
      int start,end;
      start=get_number(s);
      end=get_number(s);
      for (j=start;j<=end;j++)
      {
	sprintf(imname,"%s%04d.pcx",t,j);
	seq[i++]=cash.reg(filename,imname,SPEC_CHARACTER,1);
      }           
      get_token(s,t);      // right paren
    }
    else
      seq[i++]=cash.reg(filename,t,SPEC_CHARACTER,1);
  }
}*/





