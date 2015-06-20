#include "parse.hpp"
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

char *ttype[]={"END","NUMBER","STRING","WORD","OPERATOR","LEFT BRACE","RIGHT BRACE",
	        "LEFT PAREN","RIGHT_PAREN","ASSIGNMENT","COMMA"};

void match_right(char *&s)
{
  while (1)
  {    
    switch (token_type(s))
    {
      case sLEFT_PAREN : next_token(s); match_right(s); break;
      case sRIGHT_PAREN : next_token(s); return ; break;
      default :
        next_token(s);           
    }
  }   
}


void expect(int thing, int type, char *where)
{
  if (thing!=type)
  {
    printf("Expecting %s (not %s) at %s\n",ttype[type],ttype[thing],where);
    exit(1);
  }  
}
  

int token_type(char *s);


void skip_space(char *&s)
{
  while (*s && (*s==' ' || *s=='\n' || *s=='\t' || *s=='\r')) s++;
  
  if (*s=='/' && s[1]=='*')
  {
    s+=2;
    while (*s && (*s!='*' || s[1]!='/')) s++;
    s+=2;
    skip_space(s);    
  }    
}


int get_token(char *&s, char *buffer)  // returns token type
{
  // skip any starting spaces
  skip_space(s);

  switch (*s)
  {
    case 0    : *buffer=0; return sEND;                                  break;    
    case '{'  : *(buffer++)=*(s++); *buffer=0; return sLEFT_BRACE;       break;      
    case '}'  : *(buffer++)=*(s++); *buffer=0; return sRIGHT_BRACE;      break;
    case '('  : *(buffer++)=*(s++); *buffer=0; return sLEFT_PAREN;       break;
    case ')'  : *(buffer++)=*(s++); *buffer=0; return sRIGHT_PAREN;      break;
    case '='  : *(buffer++)=*(s++); *buffer=0; return sASSIGNMENT;       break;
    case ','  : *(buffer++)=*(s++); *buffer=0; return sCOMMA;            break;
    case '+' :
    case '-' :
    case '*' :	      
    case '/' : *(buffer++)=*(s++); *buffer=0; return sOPERATOR;          break;
    default :
    {		  
      if (isdigit(*s))
      {
	while (isdigit(*s))
	*(buffer++)=*(s++);
	*buffer=0;    
	return sNUMBER;    
      } else if (*s=='"')
      {
	while (*s=='"')
	{	  
	  s++;	
	  while (*s && *s!='"')
	    *(buffer++)=*(s++);
   	  if (*s) s++;
	  skip_space(s);
	}
	*buffer=0;    
	return sSTRING;    
      } else
      {
	*(buffer++)=*(s++);      // take the first character, no matter what it is
	                         // because nobody else will 	
	while (*s && isalnum(*s) || *s=='_' || *s=='.')
	*(buffer++)=*(s++);
	*buffer=0;
	return sWORD;    
      }
    }
  }  
}  


int token_type(char *s)
{
  char tmp[100];
  return get_token(s,tmp);  
}


void next_token(char *&s)
{
  char tmp[100];
  get_token(s,tmp);  
}

int get_number(char *&s)
{
  char tmp[100];
    
  if (get_token(s,tmp)==sOPERATOR) 
  {
    if (tmp[0]=='-')    
    {
      expect(get_token(s,tmp),sNUMBER,s);     
      return -atoi(tmp);
    }
    else expect(sWORD,sNUMBER,s);
  }
  return atoi(tmp);
}


void get_filename(char *&s, char *buffer)
{
  char *b=buffer;
  skip_space(s);
  while (*s && *s!=' ' && *s!='\n' && *s!='\t' && *s!='\r' && *s!=')' && *s!='(')
    *(b++)=*(s++);
  *b=0;
}













