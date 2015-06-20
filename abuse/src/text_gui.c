#include "lisp.hpp"
#include "lisp_gc.hpp"
#include <ctype.h>

#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#if defined( __POWERPC__ )

#elif defined( __WATCOMC__ )
#include <sys\types.h>
#include <direct.h>
#define make_dir(dir) mkdir(dir)
#else
#include <sys/stat.h>
#define make_dir(dir) mkdir(dir,S_IRWXU | S_IRWXG | S_IRWXO)

void modify_install_path(char *path) { ; }
#endif


#ifdef __WATCOMC__
void modify_install_path(char *path)
{
  char ret[100],*r,*i_path=path;
  int dc=0;
  r=ret;

  if (path[1]==':')   // if "c:\blahblah "  skip the c:
  {
    r[0]=path[0];
    r[1]=path[1];
    r+=2;
    path+=2;
  }

  while (*path)       // eliminate double slashes and reduce more than 8 char dirs
  {
    
    if (*path=='\\' || *path=='/')
    {
      dc=0;
      *r=*path;
      r++;
      path++;
      while (*path=='\\' || *path=='/') path++;
    } else if (dc<8)
    {
      dc++;
      *r=*path;
      r++;
      path++;
    } else path++;
  }


  *r=0;
  strcpy(i_path,ret);
}

#include <dos.h>
#endif

int change_dir(char *path)
{
#ifdef __WATCOMC__
  unsigned cur_drive;
  _dos_getdrive(&cur_drive);
  if (path[1]==':')
  {
    unsigned total;
    _dos_setdrive(toupper(path[0])-'A'+1,&total);


    unsigned new_drive;
    _dos_getdrive(&new_drive);

    if (new_drive!=toupper(path[0])-'A'+1)
    {
      return 0;
    }

    path+=2;
  }
  
  int er=chdir(path);
  if (er)
  {
    unsigned total;
    _dos_setdrive(cur_drive,&total);
  }
  return !er;
#else
  int ret=chdir(path);    // weird
  ret=chdir(path);
  return ret==0;
#endif  
}

long K_avail(char *path);

#ifdef __WATCOMC__
#include "i86.h"
#include <conio.h>
#include <bios.h>

long K_avail(char *path)
{
  unsigned drive=0;
  if (path[1]==':') drive=toupper(path[0])-'A'+1;
  else _dos_getdrive(&drive);

  struct diskfree_t f;
  _dos_getdiskfree(drive,&f);
  
  return (long)((long)f.sectors_per_cluster*(long)f.bytes_per_sector/1024)*(long)f.avail_clusters;
}

void set_cursor(int x, int y) {
  union  REGS in,out;
  memset(&in,0,sizeof(in));
  in.w.ax=0x0200; in.w.bx=0; in.w.dx=(y<<8)|x;
  int386(0x10,&in,&in);
}
 
void put_char(int x, int y, int val, int color) { *((unsigned short *)(0xb8000+y*2*80+x*2))=(val)|(color<<8); }
unsigned short get_char(int x, int y, int val) { return *((unsigned short *)(0xb8000+y*2*80+x*2)); }
void put_string(int x,int y,char *s, int c) { while (*s) put_char(x++,y,*(s++),c); }
void bar(int x1, int y1, int x2, int y2, int v, int c)
{ int x,y; 
  for (x=x1;x<=x2;x++)
    for (y=y1;y<=y2;y++)
      put_char(x,y,v,c);
}

void cls() { bar(0,0,79,25,' ',0x07); set_cursor(0,0); }

void box(int x1, int y1, int x2, int y2, int c)
{ 
  unsigned char bc[]={  218,191,217,192,196,179 };
  put_char(x1,y1,bc[0],c);
  put_char(x2,y1,bc[1],c);
  put_char(x2,y2,bc[2],c);
  put_char(x1,y2,bc[3],c);
  int x; for (x=x1+1;x<x2;x++) { put_char(x,y1,bc[4],c);  put_char(x,y2,bc[4],c); }
  int y; for (y=y1+1;y<y2;y++) { put_char(x1,y,bc[5],c);  put_char(x2,y,bc[5],c); }
}

void put_title(char *t)
{
  int x1=0,y1=0,x2=79,y2=25;
  bar(x1,y1,x2,y1,' ',0x4f);
  put_string((x1+x2)/2-strlen(t)/2,y1,t,0x4f);
}




int nice_copy(char *title, char *source, char *dest)
{
  int x1=0,y1=0,x2=79,y2=25;
  bar(x1,y1+1,x2,y2,176,0x01);
  put_title(title);

  box(x1,(y1+y2)/2-1,x2,(y1+y2)/2+3,0x17);
  bar(x1+1,(y1+y2)/2,x2-1,(y1+y2)/2+2,' ',0x17);
  
  char msg[100];
  sprintf(msg,"Copying %s -> %s",source,dest);
  put_string(x1+1,(y1+y2)/2,msg,0x17);
  bar(x1+1,(y1+y2)/2+2,x2-1,(y1+y2)/2+2,176,0x17);

  char *buffer=(char *)jmalloc(0xf000,"read buf");
  if (!buffer) return 0;
  FILE *out=fopen(dest,"wb");
  if (!out) { jfree(buffer) ; return 0; }
  FILE *in=fopen(source,"rb");
  if (!in) { jfree(buffer); fclose(out); unlink(dest); return 0; }

  fseek(in,0,SEEK_END);
  long size=ftell(in);
  fseek(in,0,SEEK_SET);
  int osize=size;
  while (size)
  {
    long tr=fread(buffer,1,0xf000,in);
    bar(x1+1,(y1+y2)/2+2,x1+1+(x2-x1)*(osize-size-tr)/osize,(y1+y2)/2+2,178,0x17);

    if (fwrite(buffer,1,tr,out)!=tr)
    {
      fclose(out);
      fclose(in);
      unlink(dest);
      jfree(buffer);
      return 0;
    }
    size-=tr;
    
  }
  fclose(in);
  fclose(out);
  jfree(buffer);
  cls();
  return 1;
}

void *nice_input(char *t, char *p, char *d)
{  

  int x1=0,y1=0,x2=79,y2=25;
  bar(x1,y1+1,x2,y2,176,0x01);
  put_title(t);

  box(x1,(y1+y2)/2-1,x2,(y1+y2)/2+1,0x17);
  bar(x1+1,(y1+y2)/2,x2-1,(y1+y2)/2,' ',0x17);

  put_string(x1+1,(y1+y2)/2,p,0x17);
  bar       (x1+1+strlen(p)+1,(y1+y2)/2,x2-1,(y1+y2)/2,' ',0x0f);
  put_string(x1+1+strlen(p)+1,(y1+y2)/2,d,0x70);
  set_cursor(x1+1+strlen(p)+1,(y1+y2)/2);
  while (!_bios_keybrd(_KEYBRD_READY));
  if (_bios_keybrd(_KEYBRD_READY)==7181)
  {
    getch();
    cls();
    char ln[100];
    strcpy(ln,d);                  // d might get collect in next new
    return new_lisp_string(ln);
  }
  else
  {
    bar       (x1+1+strlen(p)+1,(y1+y2)/2,x2-1,(y1+y2)/2,' ',0x0f);
    int key;
    char ln[100];
    ln[0]=0;
    do
    {
      key=getch();
      if (key==8 && ln[0]) 
      {
	ln[strlen(ln)-1]=0;
	bar(x1+1+strlen(p)+1,(y1+y2)/2,x2-1,(y1+y2)/2,' ',0x0f);
	put_string(x1+1+strlen(p)+1,(y1+y2)/2,ln,0x0f);
      }
      else if (isprint(key))
      {
	ln[strlen(ln)+1]=0;
	ln[strlen(ln)]=key;

	put_string(x1+1+strlen(p)+1,(y1+y2)/2,ln,0x0f);
      }      
      set_cursor(x1+1+strlen(p)+1+strlen(ln),(y1+y2)/2);
    } while (key!=13 && key!=27);

    cls();
    if (key==27) return NULL;
    return new_lisp_string(ln);          
  }
}

void *nice_menu(void *main_title, void *menu_title, void *list)
{
  int x1=0,y1=0,x2=79,y2=25;

  p_ref r1(main_title),r2(menu_title),r3(list);
  main_title=eval(main_title);
  menu_title=eval(menu_title);
  char *t=lstring_value(main_title);
  put_title(t);

  bar(x1,y1+1,x2,y2,176,0x01);

  void *l=eval(list);
  p_ref r4(l);
  int menu_height=list_length(l),menu_length=0;
  void *m;
  for (m=l;m;m=CDR(m))
  { long l=strlen(lstring_value(CAR(m)));
    if (l>menu_length) menu_length=l;
  }

  char *mt=lstring_value(menu_title);
  if (strlen(mt)>menu_length) menu_length=strlen(mt);

  int mx=(x2+x1)/2-menu_length/2-1,my=(y2+y1)/2-menu_height/2-2;
  box(mx,my,mx+menu_length+2,my+menu_height+3,0x17);
  bar(mx+1,my+1,mx+menu_length+1,my+menu_height+2,' ',0x17);
  put_string(mx+1+menu_length/2-strlen(mt)/2,my,mt,0x1f);       // draw menu title
  set_cursor(0,25);

  int cur_sel=0;
  int key;
  do
  {
    int y=0;
    for (m=l;m;m=CDR(m),y++)
    {
      char *s=lstring_value(CAR(m));
      if (y==cur_sel)
      {
	bar(mx+1,my+2+y,mx+1+menu_length,my+2+y,' ',0x0f);
	put_string(mx+1+menu_length/2-strlen(s)/2,my+2+y,s,0x0f);
      }
      else
      {
	bar(mx+1,my+2+y,mx+1+menu_length,my+2+y,' ',0x1f);
	put_string(mx+1+menu_length/2-strlen(s)/2,my+2+y,s,0x1f);
      }
    }

    key=_bios_keybrd(_KEYBRD_READ);

    if (key==18432) 
    { cur_sel--; if (cur_sel==-1) cur_sel=menu_height-1; }
    if (key==20480)
    { cur_sel++; if (cur_sel==menu_height) cur_sel=0; }

  } while (key!=283 && key!=7181);
  cls();
  if (key==283) 
    return new_lisp_number(-1);

  return new_lisp_number(cur_sel);
}

void center_tbox(void *list, int c)
{
  int x1=0,y1=0,x2=79,y2=25;
  int h;

  if (item_type(list)==L_CONS_CELL)
    h=list_length(list);
  else h=1;

  int y=(y1+y2)/2-h/2+1;
  box(x1,(y1+y2)/2-h/2,x2,(y1+y2)/2-h/2+h+1,c);
  bar(x1+1,y,x2-1,(y1+y2)/2-h/2+h+1-1,' ',c);

  if (item_type(list)==L_CONS_CELL)
  {
    while (list)
    {
      char *s=lstring_value(CAR(list));
      put_string((x1+x2)/2-strlen(s)/2,y++,s,c);
      list=CDR(list);
    }
  } else 
  {
    char *s=lstring_value(list);
    put_string((x1+x2)/2-strlen(s)/2,y++,s,c);
  }
}

void *show_yes_no(void *t, void *msg, void *y, void *n)
{
  p_ref r1(t),r2(msg),r3(y),r4(n);
  y=eval(y);
  n=eval(n);
  put_title(lstring_value(eval(t)));

  int x1=0,y1=0,x2=79,y2=25;
  bar(x1,y1+1,x2,y2,176,0x01);  
  center_tbox(eval(msg),0x1f);
  int key;
  char *yes=lstring_value(y);
  char *no=lstring_value(n);
  set_cursor(0,25);
  do
  {
    key=getch();
    set_cursor(0,0);
  } while (toupper(key)!=toupper(yes[0]) && toupper(key)!=toupper(no[0]));
  cls();
  if (toupper(key)==toupper(yes[0]))
    return true_symbol;
  else return NULL;
}

#else

int nice_copy(char *title, char *source, char *dest) { return 0; }

long K_avail(char *path)
{
#if 0 // ugh
  char cmd[100];
  sprintf(cmd,"du %s",path);
  FILE *fp=popen(cmd,"rb");
  if (!fp)
  {
    pclose(fp);
    return 20000;
  }
  else
  {
    fgets(cmd,100,fp);
    if (feof(fp))
    {
      pclose(fp);
      return 20000;
    }
    fgets(cmd,100,fp);
    char *s=cmd+strlen(cmd)-1;
    while (*s==' ' || *s=='\t') s--;
    while (*s!=' ' && *s!='\t') s--;  // skip last field

    while (*s==' ' || *s=='\t') s--;
    while (*s!=' ' && *s!='\t') s--;  // skip last field

    while (*s==' ' || *s=='\t') s--;
    while (*s!=' ' && *s!='\t') s--; s++;  // skip last field

    long a;
    sscanf(s,"%d",&a);
    
    pclose(fp);
    return a;
  }
#endif
}

void *show_yes_no(void *t, void *msg, void *y, void *n)
{
  p_ref r1(t),r2(msg),r3(y),r4(n);
  t=eval(t); msg=eval(msg); y=eval(y); n=eval(n);
  int c;
  char *yes=lstring_value(y);
  char *no=lstring_value(n);
  do
  {
    printf("\n\n\n\n\n%s\n\n",lstring_value(t));
    void *v=msg;
    if (item_type(v)==L_CONS_CELL)
      while (v) { printf("** %s\n",lstring_value(CAR(v))); v=CDR(v); }
    else printf("** %s\n",lstring_value(v));
    char msg[100];
    fgets(msg,100,stdin);
    c=msg[0];
  } while (toupper(c)!=toupper(yes[0]) && toupper(c)!=toupper(no[0]));
  if (toupper(c)==toupper(yes[0]))
    return true_symbol;
  else return NULL;
}

void *nice_menu(void *main_title, void *menu_title, void *list)
{
  p_ref r1(main_title),r2(menu_title),r3(list);
  main_title=eval(main_title);  menu_title=eval(menu_title);  list=eval(list);
  char msg[100];
  int i=0,d;
  do
  {
    printf("\n\n\n\n\n%s\n\n%s\n-----------------------------------\n",
	   lstring_value(main_title),lstring_value(menu_title));

    void *v=list;
    for (;v;v=CDR(v),i++)
    {
      printf("%d) %s\n",i+1,lstring_value(CAR(v)));
    }
    fprintf(stderr,"> ");
    fgets(msg,100,stdin);
    sscanf(msg,"%d",&d);
  } while (d-1>=i && d<=0);
  return new_lisp_number(d-1);
}

void *nice_input(char *t, char *p, char *d)
{
  int x; for (x=0;x<(40-strlen(t)/2);x++) printf(" ");
  printf("%s\n",t);
  for (x=0;x<78;x++) printf("-"); printf("\n");
  fprintf(stderr,"%s (ENTER=%s) > ",p,d);
  char ln[100];
  fgets(ln,100,stdin); ln[strlen(ln)-1]=0;
  if (ln[0]==0)
  {
    strcpy(ln,d);                  // d might get collect in next new
    return new_lisp_string(ln);
  }
  else
    return new_lisp_string(ln);    
}

#endif
