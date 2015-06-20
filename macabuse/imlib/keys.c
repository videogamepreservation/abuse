#include "keys.hpp"
#include <string.h>
#include <ctype.h>

char *jk_key_names[]= {"Up","Down","Left","Right",
                    "Left Ctrl","Right Ctrl","Left Alt","Right Alt",
                    "Left Shift","Right Shift","Caps Lock","Num Lock",
                    "Home","End","Del","F1","F2","F3","F4","F5","F6",
                    "F7","F8","F9","F10","Insert","PageUp","PageDown","Command"};


void key_name(int key, char *buffer)
{
  static char sing[2];
 if (key>255 && key<=JK_MAX_KEY)
    strcpy(buffer,jk_key_names[key-256]);
  else if (key==JK_BACKSPACE)
    strcpy(buffer,"Backspace");
  else if (key==JK_TAB)
    strcpy(buffer,"Tab");
  else if (key==JK_ENTER)
    strcpy(buffer,"Enter");
  else if (key==JK_ESC)
    strcpy(buffer,"Esc");
  else if (key==JK_SPACE)
    strcpy(buffer,"Space");
  else if (isprint(key))
  {
    buffer[0]=key;
    buffer[1]=0;
  } else buffer[0]=0;
}




