#include "macs.hpp"
#include "status.hpp"
#include "dprint.hpp"

status_manager *stat_man=NULL;

class text_status_node
{
  public :  
  char *name;
  text_status_node *next;
  visual_object *show;
  int last_update;
  text_status_node(char *Name, visual_object *Show, text_status_node *Next) 
  { name=strcpy((char *)jmalloc(strlen(Name)+1,"status name"),Name); 
    show=Show;
    next=Next; 
    last_update=0;
  }
  ~text_status_node() { jfree(name); if (show) delete show; }
} ; 



text_status_manager::text_status_manager()
{ 
  first=NULL; 
  level=0; 
}

void text_status_manager::push(char *name, visual_object *show)
{
  level++;
  first=new text_status_node(name,show,first);  
}

void text_status_manager::update(int percentage)
{
//	return;
  if (level==1 && percentage-first->last_update>4)
  {
  	char s[256], len;
    first->last_update=percentage;
    sprintf(s,"\r%s [",first->name);
    len = strlen(s);
    int t=percentage*40/100;
    int i=0;
    for (;i<t;i++)
      s[len+i] = '.';
    for (;i<40;i++)
    	s[len+i] = ' ';
    s[len+i++] = ']';
    s[len+i] = 0;
    dprintf("%s",s);
  }
}

void text_status_manager::pop()
{  
  CONDITION(first,"No status's to pop!");
  if (level==1) dprintf("\n");
  level--;
  text_status_node *p=first; first=first->next;
  delete p;
}





