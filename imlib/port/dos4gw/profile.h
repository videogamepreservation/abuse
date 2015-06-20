/****************************************************************************
*
*  File              : profile.h
*  Date Created      : 11/24/94
*  Description       : header file for .ini file management
*
*  Programmer(s)     : Nick Skrepetos
*  Last Modification : 11/25/94 - 11:14:58 PM
*  Additional Notes  :
*
*****************************************************************************
*            Copyright (c) 1993-95,  HMI, Inc.  All Rights Reserved            *
****************************************************************************/

#ifndef  _HMI_INI_MANAGER
#define  _HMI_INI_MANAGER

// profile structure
typedef  struct   _tagINIInstance
         {

            W32  wFlags;           // misc. flags
            BYTE  szName[ 128 ];    // name of .ini file

            PSTR  pData;            // pointer to .ini file in memory
            W32  wSize;            // size, in bytes, of file
            W32  wMaxSize;         // maximum size in bytes of the .ini

            PSTR  pCurrent;         // current location in file
            W32  wCurrent;         // current location in file

            PSTR  pSection;         // pointer to section start

            PSTR  pItemPtr;         // pointer to the start of line w/item 
            PSTR  pItem;            // pointer to last item
            PSTR  pList;            // pointer to last item location, for list
                                    // management.
            PSTR  pListPtr;         // pointer for raw string list

         } _INI_INSTANCE;

// equates
#define  _INI_SECTION_START   '['
#define  _INI_SECTION_END     ']'
#define  _INI_EQUATE          '='
#define  _INI_SPACE           ' '
#define  _INI_TAB             0x9
#define  _INI_STRING_START    '"'
#define  _INI_STRING_END      '"'
#define  _INI_EOL             0x0d
#define  _INI_CR              0x0d
#define  _INI_LF              0x0a
#define  _INI_HEX_INDICATOR   'x'
#define  _INI_LIST_SEPERATOR  ','

// amount of bytes to allocate in addition to file size so that the
// .ini file may be modified by the application.
#define  _INI_EXTRA_MEMORY    1024

// various flags for .ini structure
#define  _INI_MODIFIED        0x8000

#endif

// function prototypes
BOOL	cdecl hmiINIOpen              ( _INI_INSTANCE * sInstance, char *szName );
BOOL	cdecl hmiINIClose             ( _INI_INSTANCE * sInstance );
BOOL  cdecl hmiINILocateSection     ( _INI_INSTANCE * sInstance, PSTR szName );
BOOL	cdecl hmiINILocateItem        ( _INI_INSTANCE * sInstance, PSTR szItem );
BOOL	cdecl hmiINIGetDecimal        ( _INI_INSTANCE * sInstance, W32 * wValue );
BOOL	cdecl hmiINIGetString         ( _INI_INSTANCE * sInstance, PSTR pString, W32 wMaxLength );
BOOL	cdecl hmiINIGetQuery          ( _INI_INSTANCE * sInstance, PSTR szItem );
BOOL	cdecl hmiINIWriteDecimal      ( _INI_INSTANCE * sInstance, W32 wValue );
BOOL	cdecl hmiINIWriteString       ( _INI_INSTANCE * sInstance, PSTR szString );
BOOL	cdecl hmiINIWriteQuery        ( _INI_INSTANCE * sInstance, PSTR szItem, BOOL	wState );
BOOL	cdecl hmiINIDeleteItem        ( _INI_INSTANCE * sInstance, PSTR szItem );
BOOL	cdecl hmiINIDeleteSection     ( _INI_INSTANCE * sInstance, PSTR szSection );
BOOL	cdecl hmiINIGetRawString      ( _INI_INSTANCE * sInstance, PSTR pString, W32 wMaxLength );
BOOL	cdecl hmiINIAddSection        ( _INI_INSTANCE * sInstance, PSTR szSection );
BOOL	cdecl	hmiINIAddItemString     ( _INI_INSTANCE * sInstance, PSTR szItem, PSTR szString, W32 wJustify );
BOOL	cdecl hmiINIGetItemDecimal    ( _INI_INSTANCE * sInstance, PSTR szItem, W32 * wValue );
BOOL	cdecl	hmiINIGetItemString     ( _INI_INSTANCE * sInstance, 
                                       PSTR  szItem,
                                       PSTR  pString,
                                       W32  wMaxSize );
BOOL	cdecl	hmiINIAddItemDecimal    (   _INI_INSTANCE * sInstance,
                                        PSTR  szItem, 
                                        W32  wValue, 
                                        W32  wJustify,
                                        W32  wRadix );

