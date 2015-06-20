/****************************************************************************
*
*  File              : profile.c
*  Date Created      : 10/14/94
*  Description       : 
*
*  Programmer(s)     : Nick Skrepetos
*  Last Modification : 12/10/94 - 11:56:16 PM
*  Additional Notes  :
*
*****************************************************************************
*            Copyright (c) 1993-95,  HMI, Inc.  All Rights Reserved            *
****************************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <dos.h>
#include <fcntl.h> 
#include <bios.h>
#include <io.h>
#include <malloc.h>
#include <conio.h>
#include <ctype.h>
#include <string.h>
#include "sos.h"
#include "profile.h"

/****************************************************************************

   Possible Additions:

      hmiINIAddItemDecimal( )       adds new item w/decimal
      hmiINIAddItemQuery( )         adds new item w/query Yes/No
      hmiINIAddRawString( )         adds new raw string

   Error Checking:

      allow TABS to be skipped as "White Space" in addition to spaces so
      that editors that use tabs will work also.

      allow String functions to support lists with comma (,) seperators
      like the decimal functions do.

      allow location functions to be case sensitive or insensitive

      get rid of compiler dependent stuff like memcpy, stricmp

****************************************************************************/

// local data
static   PSTR  szHexNumbers  =  "0123456789ABCDEF";
static   W32  wMultiplier[]   =  { 1, 16, 256, 4096, 65536, 1048576, 16777216, 268435456 };

// local function prototypes
W32	hmiINIHex2Decimal    ( PSTR szHexValue );
W32	hmiINIGetHexIndex    ( BYTE bValue );


/****************************************************************************
*
*  Syntax
*
*     BOOL	cdecl hmiINIOpen( _INI_INSTANCE * sInstance, PSTR szName )
*
*  Description
*
*     opens and instance of a .ini file
*
*  Parameters
*
*        Type           Description
*        --------------------------
*        sInstance      pointer to an .ini instance
*        szName         pointer to the name of the .ini file
*
*  Return
*
*     _TRUE       the file was opened correctly
*     _FALSE      there was a problem opening the file
*
****************************************************************************/
BOOL	cdecl hmiINIOpen( _INI_INSTANCE * sInstance, PSTR szName )
	{
      W32  hFile;

      // save the name of the .ini file
      strcpy( ( char * )sInstance->szName, ( char const * )szName );

      // open .ini file, return error if file is not
      // found.
      if ( ( hFile = open( szName, O_RDONLY | O_BINARY ) ) == -1 )
         return( _FALSE );

      // determine size of file
      sInstance->wSize  =  lseek( hFile, 0, SEEK_END );

      // set the new maximum size 
      sInstance->wMaxSize  =  sInstance->wSize + _INI_EXTRA_MEMORY;

      // seek back to start of file
      lseek( hFile, 0, SEEK_SET );

      // allocate memory for the file
      if ( ( sInstance->pData  =  ( PSTR )malloc( sInstance->wMaxSize ) ) == _NULL )
      {
         // close file
         close( hFile );

         // return error, not enough memory
         return( _FALSE );
      }

      // read in file
      if ( read( hFile, sInstance->pData, sInstance->wSize ) != sInstance->wSize )
      {
         // close file
         close( hFile );

         // free memory
         free( sInstance->pData );

         // return error, not file size incorrect
         return( _FALSE );
      }

      // close file
      close( hFile );

      // init current position
      sInstance->pCurrent  =  sInstance->pData;
      sInstance->wCurrent  =  0;

      // initalize current item pointer
      sInstance->pItem     =  _NULL;
      sInstance->pList     =  _NULL;
      sInstance->pItemPtr  =  _NULL;
      sInstance->pListPtr  =  _NULL;

      // reset the modified flag to indicate that the 
      // file is unmodified.
      sInstance->wFlags    &= ~_INI_MODIFIED;

      // return success
      return( _TRUE );
	}

/****************************************************************************
*
*  Syntax
*
*     BOOL	cdecl hmiINIClose( _INI_INSTANCE * sInstance )
*
*  Description
*
*     close and instance of a .ini file. note that if the file is modified
*     it will be written back to the original file.
*
*  Parameters
*
*        Type           Description
*        --------------------------
*        sInstance      pointer to .ini instance
*
*  Return
*
*     _TRUE       file was closed and/or written correctly
*     _FALSE      a problem was encountered when writing/closing the file
*
****************************************************************************/
BOOL	cdecl hmiINIClose( _INI_INSTANCE * sInstance )
	{
      W32  hFile;

      // determine if the .ini file has been modified
      if ( sInstance->wFlags & _INI_MODIFIED )
      {
         // create and open file 
         if ( ( hFile =  open( (const char * )sInstance->szName, O_CREAT | O_TRUNC | O_RDWR | O_BINARY, 0 ) ) == -1 )
         {
      		// free memory
            free( sInstance->pData );

            // error creating file
            return( _FALSE );
         }

         // write data back out
         write( hFile, sInstance->pData, sInstance->wSize );

         // close file
         close( hFile );
      }

		// free memory
      free( sInstance->pData );

      // return success
      return( _TRUE );
	}


/****************************************************************************
*
*  Syntax
*
*     BOOL cdecl  hmiINILocateSection( _INI_INSTANCE * sInstance, PSTR szName )
*
*  Description
*
*     locates a section in a file.  a section is determined by enclosing 
*     it in [].  ie.  [SECTION]
*
*  Parameters
*
*        Type           Description
*        --------------------------
*        sInstance      pointer to .ini instance
*        szName         pointer to section name
*
*  Return
*
*     _TRUE       section located
*     _FALSE      section not located
*
****************************************************************************/
BOOL  cdecl hmiINILocateSection( _INI_INSTANCE * sInstance, PSTR szName )
	{
      PSTR  pDataPtr;
      PSTR	pSectionPtr;
      PSTR  szSection;
      W32  wIndex;
      W32  wFoundFlag  =  _FALSE;

      // set data pointer to pointer to start of .ini in memory
      pDataPtr    =  sInstance->pData;

      // initialize index
      wIndex      =  0;

      // search data until we have found a start section character
      // and then attempt to match section string.  continue to process
      // entire data set until the end is reached or a match is 
      // found.
      do
         {
            // check if character we are pointing to is a 
            // start section character
            if ( *pDataPtr == _INI_SECTION_START )
            {
               // save pointer to start of section for use by the
               // delete functions.
               pSectionPtr =  pDataPtr;

               // advance past the start section character
               pDataPtr++;

               // set pointer to section name
               szSection   =  szName;

               // search the string character by character to determine
               // if we have a match.
               while( *pDataPtr == *szSection && wIndex < sInstance->wSize )
               {
                  // advance section pointer
                  szSection++;

                  // advance data pointer
                  pDataPtr++;

                  // advance data index
                  wIndex++;
               }

               // determine if we are sitting on a end section 
               // character. if so then we have a complete match
               // so set the found flag to true.
               if ( *pDataPtr == _INI_SECTION_END && *szSection == _NULL )
               {
                  // set found flag
                  wFoundFlag  =  _TRUE;

                  // move to the next line
                  while( *pDataPtr != _INI_LF )
                     pDataPtr++;

                  // advance past line feed
                  pDataPtr++;

                  // set list pointer for raw name
                  sInstance->pListPtr  =  pDataPtr;

                  // set current data pointer to new section
                  // location.
                  sInstance->pCurrent  =  pDataPtr;
                  sInstance->wCurrent  =  wIndex;

                  // save pointer to start of current section for
                  // use by other functions.
                  sInstance->pSection  =  pSectionPtr;
               }

            }

            // advance pointer 
            pDataPtr++;

            // advance index
            wIndex++;
         }
      while( !wFoundFlag && wIndex < sInstance->wSize );

      // return the status of the found flag, this will indicate
      // if the desired section was located.
      return( ( BOOL )wFoundFlag );
	}


/****************************************************************************
*
*  Syntax
*
*     BOOL	cdecl hmiINILocateItem( _INI_INSTANCE * sInstance, PSTR szItem )
*
*  Description
*
*     locates an item under a section in an .ini file
*
*  Parameters
*
*        Type           Description
*        --------------------------
*        sInstance      pointer to an .ini instance
*        szItem         pointer to the name of the item
*
*  Return
*
*     _TRUE    the item was located
*     _FALSE   the item was not located
*
****************************************************************************/
BOOL	cdecl hmiINILocateItem( _INI_INSTANCE * sInstance, PSTR szItem )
	{
      PSTR  pDataPtr;
      PSTR  pItemPtr;
      PSTR  szSearch;
      W32  wIndex;
      W32  wFoundFlag  =  _FALSE;

      // initialize current location pointers
      pDataPtr =  sInstance->pCurrent;
      wIndex   =  sInstance->wCurrent;

      // search each data item until match is found or the start
      // of a new section is found
      do
         {
            // set up search pointer
            szSearch =  szItem;

            // check if current character matches first
            // character of search string
            if ( *pDataPtr == *szSearch )
            {
               // set pointer to start of potential item.  this pointer
               // will be used later to mark the start of the item
               // string.
               pItemPtr    =  pDataPtr;

               // advance data pointer and search pointer
               pDataPtr++;
               szSearch++;

               // advance search index
               wIndex++;

               // search the rest of the string, make sure we do not overrun
               // the end of file and that the string still matches.
               while( *pDataPtr == *szSearch && wIndex < sInstance->wSize )
               {
                  // advance data pointer
                  pDataPtr++;

                  // advance search pointer
                  szSearch++;

                  // advance index
                  wIndex++;
               }

               // check if we located the string
               if ( *szSearch == _NULL )
               {
                  // skip any white until we locate the '='
                  // sign.
                  while( *pDataPtr != _INI_EQUATE && *pDataPtr != _INI_EOL )
                  {
                     // advance data pointer
                     pDataPtr++;

                     // advance index
                     wIndex++;
                  }

                  // check if we found and equate '=' character, if not
                  // only set the start of line indicator so the string
                  // and decimal routines know there is no value.
                  if ( *pDataPtr == _INI_EQUATE )
                  {
                     // advance data pointer one past the equate
                     pDataPtr++;

                     // advance index
                     wIndex++;

                     // set the pointer to the new item
                     sInstance->pItem     =  pDataPtr;
                  }
                  else
                     sInstance->pItem     =  _NULL;

                  // set the start of line item pointer for later use
                  sInstance->pItemPtr  =  pItemPtr;

                  // reset list pointer to indicate that we
                  // do not have a list yet.
                  sInstance->pList  =  _NULL;

                  // set the found flag
                  wFoundFlag  =  _TRUE;
               }
            }

            // advance to next place in data
            pDataPtr++;

            // advance index
            wIndex++;
         }
      while( !wFoundFlag && wIndex < sInstance->wSize && *pDataPtr != _INI_SECTION_START );

      // return found flag status
      return( ( BOOL )wFoundFlag );
	}


/****************************************************************************
*
*  Syntax
*
*     BOOL	cdecl hmiINIGetDecimal( _INI_INSTANCE * sInstance, W32 * wValue )
*
*  Description
*
*     retrieves a decimal value from an item in a .ini file.  note that if
*     the value is in hex (0x....) it will be automatically converted to 
*     decimal.
*
*  Parameters
*
*        Type           Description
*        --------------------------
*        sInstance      pointer to an .ini instance
*        wValue         pointer to a word to store decimal value in
*
*  Return
*
*     _TRUE    value was located and converted correctly
*     _FALSE   value was not valid, or item search was not performed
*
****************************************************************************/
BOOL	cdecl hmiINIGetDecimal( _INI_INSTANCE * sInstance, W32 * wValue )
	{
      PSTR  pDataPtr;
		W32  wDValue;
      BYTE  bBuffer[ 32 ];
      W32  wIndex;

      // initialize pointer to data
      if ( sInstance->pList )
         pDataPtr    =  sInstance->pList;
      else
         pDataPtr    =  sInstance->pItem;

      // check if it is null
      if ( pDataPtr == _NULL )
         return( _FALSE );

      // skip all white space
      while( *pDataPtr == _INI_SPACE )
         pDataPtr++;

      // check if we are pointing to and EOL
      if ( *pDataPtr == _INI_EOL )
         return( _FALSE );

      // initialize buffer index
      wIndex   =  0;

      // fetch string for value
      while( *pDataPtr != _INI_EOL && *pDataPtr != _INI_LIST_SEPERATOR 
               && *pDataPtr != _INI_SPACE )
      {
         // save character
         bBuffer[ wIndex++ ]  =  *pDataPtr++;
      }

      // set null at the end of buffer
      bBuffer[ wIndex ]       =  '\0';

      // check if we have simply reached the end of the
      // line with no number.
      if ( wIndex == 0 )
         return( _FALSE );

      // skip all white space
      while( *pDataPtr == _INI_SPACE )
         pDataPtr++;

      // check if we have a list of numbers
      if ( *pDataPtr == _INI_LIST_SEPERATOR )
      {
         // set list pointer to one past the current
         // seperator.
         sInstance->pList  =  ++pDataPtr;
      }
      else
         sInstance->pList  =  pDataPtr;

      // check if the buffer contains a hex value
      if ( bBuffer[ 1 ] == _INI_HEX_INDICATOR )
      {
         // fetch hex value
         wDValue  =  hmiINIHex2Decimal( ( char near * )&bBuffer[ 2 ] );
      }
      else
      {
         // fetch value
         wDValue  =  (W32)atoi( ( const char * )bBuffer );
      }

      // set value
      *wValue  =  wDValue;

      // return status
      return( _TRUE );
	}


/****************************************************************************
*
*  Syntax
*
*     BOOL	cdecl hmiINIGetString( _INI_INSTANCE * sInstance, PSTR pString, W32 wMaxLength )
*
*  Description
*
*     fetches string from .ini file
*
*  Parameters
*
*        Type           Description
*        --------------------------
*        sInstance      pointer to .ini instance
*        pString        pointer to store string
*        wMaxLength     maximum length of string to fetch
*
*  Return
*
*     _TRUE    string was returned correctly
*     _FALSE   item search was not performed before calling this function
*
****************************************************************************/
BOOL	cdecl hmiINIGetString( _INI_INSTANCE * sInstance, PSTR pString, W32 wMaxLength )
	{
      PSTR  pDataPtr;
      W32  wIndex;

      // initialize pointer to data
      if ( sInstance->pList )
         pDataPtr    =  sInstance->pList;
      else
         pDataPtr    =  sInstance->pItem;

      // check if it is null
      if ( pDataPtr == _NULL )
         return( _FALSE );

      // initialize index
      wIndex   =  0;

      // find start of string, first non-space character
      while( *pDataPtr  == _INI_SPACE )
         pDataPtr++;

      // copy string into buffer
      while( *pDataPtr != _INI_EOL && *pDataPtr != _INI_LIST_SEPERATOR && wIndex < wMaxLength - 1 )
         pString[ wIndex++ ] =  *pDataPtr++;

      // place a _NULL at the end of the string
      pString[ wIndex ] =  '\0';

      // if we have reached maximum buffer length, search until
      // we get to the EOL or list seperator
      if ( wIndex == wMaxLength - 1 )
      {
         // find end / list seperator
         while( *pDataPtr != _INI_EOL && *pDataPtr != _INI_LIST_SEPERATOR )
            pDataPtr++;
      }

      // check if we have a list of items
      if ( *pDataPtr == _INI_LIST_SEPERATOR )
      {
         // set list pointer to one past the current
         // seperator.
         sInstance->pList  =  ++pDataPtr;
      }
      else
         sInstance->pList  =  pDataPtr;

      // return success
      return( _TRUE );
	}

/****************************************************************************
*
*  Syntax
*
*     BOOL	cdecl hmiINIGetRawString( _INI_INSTANCE * sInstance, PSTR pString, W32 wMaxLength )
*
*  Description
*
*     fetches string from .ini file
*
*  Parameters
*
*        Type           Description
*        --------------------------
*        sInstance      pointer to .ini instance
*        pString        pointer to store string
*        wMaxLength     maximum length of string to fetch
*
*  Return
*
*     _TRUE    string was returned correctly
*     _FALSE   end of strings to fetch, or no section locate performed
*
****************************************************************************/
BOOL	cdecl hmiINIGetRawString( _INI_INSTANCE * sInstance, PSTR pString, W32 wMaxLength )
	{
      PSTR  pDataPtr;
      PSTR  pEOFPtr;
      W32  wIndex;

      // initialize data pointer
      pDataPtr =  sInstance->pListPtr;

      // check if it is null
      if ( pDataPtr == _NULL || *pDataPtr == _INI_SECTION_START ||
            *pDataPtr   == _INI_EOL )
         return( _FALSE );

      // determine EOF pointer
      pEOFPtr  =  sInstance->pData + sInstance->wSize;

      // initialize index
      wIndex   =  0;

      // find start of string, first non-space character
      while( *pDataPtr  == _INI_SPACE )
         pDataPtr++;

      // copy string into buffer
      while( *pDataPtr != _INI_EOL && wIndex < wMaxLength - 1 )
         pString[ wIndex++ ] =  *pDataPtr++;

      // place a _NULL at the end of the string
      pString[ wIndex ] =  '\0';

      // skip past end of line
      pDataPtr += 2;

      // make sure we are not at the end of the
      // file.
      if ( pDataPtr >= pEOFPtr )
         sInstance->pListPtr  =  _NULL;
      else
         // save off current position
         sInstance->pListPtr  =  pDataPtr;

      // return success
      return( _TRUE );
	}

/****************************************************************************
*
*  Syntax
*
*     BOOL	cdecl hmiINIWriteDecimal( _INI_INSTANCE * sInstance, W32 wValue )
*
*  Description
*
*     writes decimal value to .ini item
*
*  Parameters
*
*        Type           Description
*        --------------------------
*        sInstance      pointer to an .ini instance
*        wValue         value to write out
*
*  Return
*
*     _TRUE    value written correctly
*     _FALSE   value not written correctly
*
****************************************************************************/
BOOL	cdecl hmiINIWriteDecimal( _INI_INSTANCE * sInstance, W32 wValue )
	{
      PSTR  pDataPtr;
      PSTR  pValuePtr;
      BYTE  bBuffer[ 32 ];
      W32  wIndex;
      W32  wStringSize;
      W32  wDecimalSize;
      W32  wMoveSize;

      // check if item was previously located
      if ( sInstance->pItem == _NULL )
         return( _FALSE );

      // get pointer to item
      pDataPtr =  sInstance->pItem;

      // skip all white space
      while( *pDataPtr == _INI_SPACE )
         pDataPtr++;

      // save pointer to value location
      pValuePtr   =  pDataPtr;

      // initialize string length
      wStringSize =  0;

      // fetch string for value
      while( *pDataPtr != _INI_EOL )
      {
         // save character and advance string size
         bBuffer[ wStringSize++ ]   =  *pDataPtr++;
      }

      // set null at the end of buffer
      bBuffer[ wStringSize ]  =  '\0';

      // check if the buffer contains a hex value
      if ( bBuffer[ 1 ] == _INI_HEX_INDICATOR )
      {
         // convert value to hex string
         itoa( wValue, ( char * )&bBuffer[ 2 ], 16 );
      }
      else
      {
		   // convert value to decimal string
         itoa( wValue, ( char * )&bBuffer[ 0 ], 10 );
      }

      // get length of converted string
      wDecimalSize   =  strlen( ( const char * )bBuffer );

      // check if we need to shrink or expand the 
      // data size.
      if ( wDecimalSize < wStringSize )
      {
         // calculate the move size
         wMoveSize   =  ( ( sInstance->pData + sInstance->wSize ) - pValuePtr ) - ( wStringSize - wDecimalSize );

         // need to shrink the size of the .ini file
         memmove( pValuePtr, pValuePtr + ( wStringSize - wDecimalSize ), wMoveSize );

         // adjust size
         sInstance->wSize  -= ( wStringSize - wDecimalSize );
      }
      else
         // check if expand data
         if ( wDecimalSize > wStringSize )
         {
            // make sure we have enough memory to expand
            if ( sInstance->wSize + ( wDecimalSize - wStringSize ) > sInstance->wMaxSize )
               return( _FALSE );

            // need to expand the size of the .ini file,
            // calculate the move size
            wMoveSize   =  ( ( sInstance->pData + sInstance->wSize ) - pValuePtr ) + ( wDecimalSize - wStringSize );

            // need to shrink the size of the .ini file
            memmove( pValuePtr + ( wDecimalSize - wStringSize ), pValuePtr, wMoveSize );

            // adjust size
            sInstance->wSize  += ( wDecimalSize - wStringSize );
         }

      // initialize index
      wIndex      =  0;

      // copy in new string
      while( bBuffer[ wIndex ] )
         *pValuePtr++   =  bBuffer[ wIndex++ ];

      // set the modified flag
      sInstance->wFlags    |= _INI_MODIFIED;

      // return success
      return( _TRUE );
	}

/****************************************************************************
*
*  Syntax
*
*     BOOL	cdecl hmiINIWriteString( _INI_INSTANCE * sInstance, PSTR szString )
*
*  Description
*
*     writes out string to item
*
*  Parameters
*
*        Type           Description
*        --------------------------
*        sInstance      pointer to an .ini instance
*        szString       null terminated string
*
*  Return
*
*     _TRUE    the string was written correctly
*     _FALSE   the string was not written correctly
*
****************************************************************************/
BOOL	cdecl hmiINIWriteString( _INI_INSTANCE * sInstance, PSTR szString )
	{
      PSTR  pDataPtr;
      PSTR  pStringPtr;
      W32  wMoveSize;
      W32  wSourceSize;
      W32  wDestSize;

      // initialize data pointer
      pDataPtr =  sInstance->pItem;

      // check if it is null
      if ( pDataPtr == _NULL )
         return( _FALSE );

      // initialize destination length
      wDestSize   =  0;

      // find start of string character, first non-space
      // character.
      while( *pDataPtr  == _INI_SPACE )
         pDataPtr++;

      // save place to store new string
      pStringPtr  =  pDataPtr;

      // copy string into buffer
      while( *pDataPtr++ != _INI_EOL )
         wDestSize++;

      // get string length of new string
      wSourceSize    =  strlen( szString );

      // check if we need to shrink or expand the 
      // data size.
      if ( wSourceSize < wDestSize )
      {
         // calculate the move size
         wMoveSize   =  ( ( sInstance->pData + sInstance->wSize ) - pStringPtr ) - ( wDestSize - wSourceSize );

         // need to shrink the size of the .ini file
         memmove( pStringPtr, pStringPtr + ( wDestSize - wSourceSize ), wMoveSize );

         // adjust size
         sInstance->wSize  -= ( wDestSize - wSourceSize );
      }
      else
         // check if expand data
         if ( wSourceSize > wDestSize )
         {
            // make sure we have enough memory to expand
            if ( sInstance->wSize + ( wSourceSize - wDestSize ) > sInstance->wMaxSize )
               return( _FALSE );

            // need to expand the size of the .ini file,
            // calculate the move size
            wMoveSize   =  ( ( sInstance->pData + sInstance->wSize ) - pStringPtr ) + ( wSourceSize - wDestSize );

            // need to shrink the size of the .ini file
            memmove( pStringPtr + ( wSourceSize - wDestSize ), pStringPtr, wMoveSize );

            // adjust size
            sInstance->wSize  += ( wSourceSize - wDestSize );
         }

      // copy in new string
      while( *szString )
         *pStringPtr++   =  *szString++;

      // set the modified flag
      sInstance->wFlags    |= _INI_MODIFIED;

      // return success
      return( _TRUE );
	}

/****************************************************************************
*
*  Syntax
*
*     BOOL	cdecl hmiINIGetQuery( _INI_INSTANCE * sInstance, PSTR szItem )
*
*  Description
*
*     get a Yes/No type answer from item
*
*  Parameters
*
*        Type           Description
*        --------------------------
*        sInstance      pointer to an .ini instance
*        szItem         pointer to item to locate in query
*
*  Return
*
*     _TRUE    "Yes" was found
*     _FALSE   "No" or other was found
*
****************************************************************************/
BOOL	cdecl hmiINIGetQuery( _INI_INSTANCE * sInstance, PSTR szItem )
	{
		BYTE  bBuffer[ 32 ];

      // locate item within section
      if ( !hmiINILocateItem( sInstance, szItem ) )
         return( _FALSE );

      // get string from .ini file
      if ( !hmiINIGetString( sInstance, ( char near * )bBuffer, 32 ) )
         return( _FALSE );

      // compare string to "Yes"
      if ( strcmpi( ( char const * )bBuffer, ( char const * )"YES" ) == 0 )
         return( _TRUE );
      else
         return( _FALSE );
	}

/****************************************************************************
*
*  Syntax
*
*     BOOL	cdecl hmiINIGetItemDecimal( _INI_INSTANCE * sInstance, PSTR szItem, 
*                                       W32 * wValue )
*
*  Description
*
*     locates and item and returns the decimal value associated with it
*
*  Parameters
*
*        Type           Description
*        --------------------------
*        sInstance      pointer to .ini instance
*        szItem         pointer to item string
*        wValue         pointer to word value 
*
*  Return
*
*     _TRUE       value located and retrieved
*     _FALSE      error getting value
*
****************************************************************************/
BOOL	cdecl hmiINIGetItemDecimal( _INI_INSTANCE * sInstance, PSTR szItem, 
                                  W32 * wValue )
	{
      // attempt to locate item
      if ( !hmiINILocateItem( sInstance, szItem ) )
         return( _FALSE );

      // get decimal value
      if ( !hmiINIGetDecimal( sInstance, wValue ) )
         return( _FALSE );

      // return success
      return( _TRUE );
	}

/****************************************************************************
*
*  Syntax
*
*     BOOL	cdecl	hmiINIGetItemString( _INI_INSTANCE * sInstance, PSTR szItem,
*                                      PSTR  pString, W32 wMaxSize )
*
*  Description
*
*     locates item and retrieves string associated with it
*
*  Parameters
*
*        Type           Description
*        --------------------------
*        sInstance      pointer to an .ini instance
*        szItem         pointer to item string
*        pString        pointer to string data area
*        wMaxSize       maximum size of the string to get
*
*  Return
*
*     _TRUE       string retrieved
*     _FALSE      error retrieving string
*
****************************************************************************/
BOOL	cdecl	hmiINIGetItemString( _INI_INSTANCE * sInstance, PSTR  szItem,
                                 PSTR  pString, W32  wMaxSize )
	{
      // attempt to locate the item string
		if ( !hmiINILocateItem( sInstance, szItem ) )
         return( _FALSE );

      // attempt to get string
      if ( !hmiINIGetString( sInstance, pString, wMaxSize ) )
         return( _FALSE );

      // return success
      return( _TRUE );
	}

/****************************************************************************
*
*  Syntax
*
*     BOOL	cdecl hmiINIWriteQuery( _INI_INSTANCE * sInstance, PSTR szItem, BOOL	wState )
*
*  Description
*
*     write a Yes/No type answer to an item
*
*  Parameters
*
*        Type           Description
*        --------------------------
*        sInstance      pointer to an .ini instance
*        szItem         pointer to item to alter
*        wState         flag: _TRUE or _FALSE
*
*  Return
*
*     _TRUE    query was modified
*     _FALSE   query was not modified
*
****************************************************************************/
BOOL	cdecl hmiINIWriteQuery( _INI_INSTANCE * sInstance, PSTR szItem, BOOL	wState )
	{
      // locate item within section
      if ( !hmiINILocateItem( sInstance, szItem ) )
         return( _FALSE );

      // write string to .ini file
      if ( wState )
      {
         // write string
         if ( !hmiINIWriteString( sInstance, "Yes" ) )
            return( _FALSE );
      }
      else
      {
         // write string
         if ( !hmiINIWriteString( sInstance, "No" ) )
            return( _FALSE );
      }

      // return success
      return( _TRUE );
	}

/****************************************************************************
*
*  Syntax
*
*     BOOL	cdecl hmiINIDeleteItem( _INI_INSTANCE * sInstance, PSTR szItem )
*
*  Description
*
*     removes and item from within a section
*
*  Parameters
*
*        Type           Description
*        --------------------------
*        sInstance      pointer to and .ini instance
*        szItem         pointer to item string to remove
*
*  Return
*
*     _TRUE       item was found and removed
*     _FALSE      item was not located
*
****************************************************************************/
BOOL	cdecl hmiINIDeleteItem( _INI_INSTANCE * sInstance, PSTR szItem )
	{
      PSTR  pDataPtr;
      W32  wStrSize =  0;

      // locate item within section
      if ( !hmiINILocateItem( sInstance, szItem ) )
         return( _FALSE );

      // get pointer to data
      pDataPtr =  sInstance->pItemPtr;

      // now find the length of the string to delete
      while( *(pDataPtr + wStrSize ) != _INI_LF )
         wStrSize++;

      // add one to size to include line feed
      wStrSize++;

      // perform memory move to delete string
      memmove( pDataPtr, pDataPtr + wStrSize, ( sInstance->pData + sInstance->wSize ) - ( pDataPtr + wStrSize ) );

      // adjust size of .ini file
      sInstance->wSize  -=    wStrSize;

      // set the modified flag
      sInstance->wFlags    |= _INI_MODIFIED;

		// return success
      return( _TRUE );
	}

/****************************************************************************
*
*  Syntax
*
*     BOOL	cdecl hmiINIDeleteSection( _INI_INSTANCE * sInstance, PSTR szSection )
*
*  Description
*
*     removes and item from within a section
*
*  Parameters
*
*        Type           Description
*        --------------------------
*        sInstance      pointer to and .ini instance
*        szSection      pointer to section to remove
*
*  Return
*
*     _TRUE       section was found and removed
*     _FALSE      section was not located
*
****************************************************************************/
BOOL	cdecl hmiINIDeleteSection( _INI_INSTANCE * sInstance, PSTR szSection )
	{
      PSTR  pDataPtr;
      PSTR  pEOFPtr;
      W32  wSize;

      // locate item within section
      if ( !hmiINILocateSection( sInstance, szSection ) )
         return( _FALSE );

      // get pointer to data
      pDataPtr =  sInstance->pSection;

      // set end of file pointer
      pEOFPtr  =  sInstance->pData  +  sInstance->wSize;

      // advance szie one past section start to find the start of either the
      // next section or the end of file.
      wSize =  1;

      // now find the length of the string to delete
      while( *(pDataPtr + wSize ) != _INI_SECTION_START && ( pDataPtr + wSize ) < pEOFPtr )
         wSize++;

      // perform memory move to delete string
      memmove( pDataPtr, pDataPtr + wSize, pEOFPtr - ( pDataPtr + wSize ) );

      // adjust size of .ini file
      sInstance->wSize  -=    wSize;

      // set the modified flag
      sInstance->wFlags    |= _INI_MODIFIED;

		// return success
      return( _TRUE );
	}

/****************************************************************************
*
*  Syntax
*
*     BOOL	cdecl hmiINIAddSection( _INI_INSTANCE * sInstance, PSTR szSection )
*
*  Description
*
*     adds new section [...] to end of current file
*
*  Parameters
*
*        Type           Description
*        --------------------------
*        sInstance      pointer to an .ini instance
*        szSection      pointer to name of section to add
*
*  Return
*
*     _TRUE    section added to end of file
*     _FALSE   not enough memory to add new section or section exists
*
****************************************************************************/
BOOL	cdecl hmiINIAddSection( _INI_INSTANCE * sInstance, PSTR szSection )
	{
      PSTR  pDataPtr;
      W32  wSize;

      // check if section exists
      if ( hmiINILocateSection( sInstance, szSection ) )
         return( _FALSE );

      // get pointer to end of file
      pDataPtr =  sInstance->pData + sInstance->wSize;

      // get size of new section name to add, plus
      // 8 to account for the CR/LF, start, end, CR/LF
      wSize =  strlen( szSection ) + 6;

      // check if there is room to add section
      if ( sInstance->wSize + wSize > sInstance->wMaxSize )
         return( _FALSE );

      // add CR/LF @ the end of the file
      *pDataPtr++ =  _INI_CR;
      *pDataPtr++ =  _INI_LF;

      // set current section pointer to here
      sInstance->pSection  =  pDataPtr;

      // copy in string start
      *pDataPtr++ =  _INI_SECTION_START;

      // copy in string
      while( *szSection )
         *pDataPtr++ =  *szSection++;

      // copy in string end
      *pDataPtr++ =  _INI_SECTION_END;
		
      // add CR/LF @ the end of the file
      *pDataPtr++ =  _INI_CR;
      *pDataPtr++ =  _INI_LF;

      // set section list pointer to here
      sInstance->pListPtr  =  pDataPtr;

      // set current pointer to here
      sInstance->pCurrent  =  pDataPtr;

      // adjust size
      sInstance->wSize     += wSize;

      // set the modified flag
      sInstance->wFlags    |= _INI_MODIFIED;

      // return success
      return( _TRUE );

	}

/****************************************************************************
*
*  Syntax
*
*     BOOL	cdecl	hmiINIAddItemString( _INI_INSTANCE * sInstance, PSTR szItem, 
*                                      PSTR szString, W32 wJustify )
*
*  Description
*
*     adds new item field w/string
*
*  Parameters
*
*        Type           Description
*        --------------------------
*        sInstance      pointer to an .ini instance
*        szItem         pointer to item string
*        szString       pointer to string 
*        wJustify       width of field
*
*  Return
*
*     _TRUE    item and string added
*     _FALSE   item not added, not enough memory, etc..
*
****************************************************************************/
BOOL	cdecl	hmiINIAddItemString( _INI_INSTANCE * sInstance, PSTR szItem, PSTR szString, W32 wJustify )
	{
      PSTR  pDataPtr;
      W32  wSize;

      // check if item exists
      if ( hmiINILocateItem( sInstance, szItem ) )
      {
         // alter string
         hmiINIWriteString( sInstance, szString );

         // return success
         return( _TRUE );
      }

      // get data pointer
      pDataPtr    =  sInstance->pCurrent;

      // adjust size to account for item, =, SPACE, STRING, CR/LF
      wSize       =  wJustify + 4 + strlen( szString );

      // check if there is room to add item/string
      if ( sInstance->wSize + wSize > sInstance->wMaxSize )
         return( _FALSE );

      // make room in memory
      memmove( pDataPtr + wSize, pDataPtr, ( sInstance->pData + sInstance->wSize ) - pDataPtr );

      // copy in new item string
      while( *szItem )
      {
         // copy in character into .ini
         *pDataPtr++ =  *szItem++;

         // decrement justify characters
         wJustify--;
      }

      // fill in the rest of the white space to 
      // justify item
      while( wJustify-- )
         *pDataPtr++ =  _INI_SPACE;

      // fill in the = sign and an extra space
      *pDataPtr++    =  _INI_EQUATE;
      *pDataPtr++    =  _INI_SPACE;

      // copy in the string
      while( *szString )
         *pDataPtr++ =  *szString++;

      // add in CR/LF
      *pDataPtr++    =  _INI_CR;
      *pDataPtr++    =  _INI_LF;

      // adjust size
      sInstance->wSize     += wSize;

      // set the modified flag
      sInstance->wFlags    |= _INI_MODIFIED;

      // return success
      return( _TRUE );

	}

/****************************************************************************
*
*  Syntax
*
*     BOOL	cdecl	hmiINIAddItemDecimal( _INI_INSTANCE * sInstance, PSTR szItem, 
*                                      W32 wValue, W32 wJustify, W32 wRadix )
*
*  Description
*
*     adds new item field w/decimal
*
*  Parameters
*
*        Type           Description
*        --------------------------
*        sInstance      pointer to an .ini instance
*        szItem         pointer to item string
*        wValue         value to write out
*        wJustify       width of field
*        wRadix         radix (base) of field
*
*  Return
*
*     _TRUE    item and string added
*     _FALSE   item not added, not enough memory, etc..
*
****************************************************************************/
BOOL	cdecl	hmiINIAddItemDecimal(   _INI_INSTANCE * sInstance,
                                    PSTR  szItem, 
                                    W32  wValue, 
                                    W32  wJustify,
                                    W32  wRadix )
	{
      PSTR  pDataPtr;
      W32  wSize;
      W32  wIndex;
      BYTE  szBuffer[ 32 ];

      // check if item exists
      if ( hmiINILocateItem( sInstance, szItem ) )
      {
         // alter string
         hmiINIWriteDecimal( sInstance, wValue );

         // return success
         return( _TRUE );
      }

      // convert decimal to string
      if ( wRadix == 16 )
      {
         // add hex '0x' preceeding value
         szBuffer[ 0 ]  =  '0';
         szBuffer[ 1 ]  =  'x';

         // convert to hex
         itoa( wValue, (PSTR)&szBuffer[ 2 ], 16 );
      }
      else
         itoa( wValue, (PSTR)&szBuffer[ 0 ], 10 );

      // get data pointer
      pDataPtr    =  sInstance->pCurrent;

      // adjust size to account for item, =, SPACE, STRING, CR/LF
      wSize       =  wJustify + 4 + strlen( ( char const * )szBuffer );

      // check if there is room to add item/string
      if ( sInstance->wSize + wSize > sInstance->wMaxSize )
         return( _FALSE );

      // make room in memory
      memmove( pDataPtr + wSize, pDataPtr, ( sInstance->pData + sInstance->wSize ) - pDataPtr );

      // copy in new item string
      while( *szItem )
      {
         // copy in character into .ini
         *pDataPtr++ =  *szItem++;

         // decrement justify characters
         wJustify--;
      }

      // fill in the rest of the white space to 
      // justify item
      while( wJustify-- )
         *pDataPtr++ =  _INI_SPACE;

      // fill in the = sign and an extra space
      *pDataPtr++    =  _INI_EQUATE;
      *pDataPtr++    =  _INI_SPACE;

      // reset index
      wIndex   =  0;

      // copy in the string
      while( szBuffer[ wIndex ] )
         *pDataPtr++ =  szBuffer[ wIndex++ ];

      // add in CR/LF
      *pDataPtr++    =  _INI_CR;
      *pDataPtr++    =  _INI_LF;

      // adjust size
      sInstance->wSize     += wSize;

      // set the modified flag
      sInstance->wFlags    |= _INI_MODIFIED;

      // return success
      return( _TRUE );

	}

/****************************************************************************
*
*  Syntax
*
*     W32	hmiINIHex2Decimal( PSTR szHexValue )
*
*  Description
*
*     Converts passed number to hex
*
*  Parameters
*
*        Type           Description
*        --------------------------
*        szHexValue     pointer to the string containing hex value
*
*  Return
*
*     decimal value of hex number
*
****************************************************************************/
W32	hmiINIHex2Decimal( PSTR szHexValue )
	{
      W32  wDecimal    =  0;
      W32	wPlaces     =  strlen( szHexValue );
      W32	wMultIndex;
      W32	wIndex      =  0;

		// count down
      do
         {
            // accumulate value
            wDecimal += wMultiplier[ wPlaces - 1 ] * hmiINIGetHexIndex( (BYTE)szHexValue[ wIndex++ ] );

            // decrement places
            wPlaces--;
         }
      while( wPlaces > 0 );

      // return decimal
      return( wDecimal );
	}

/****************************************************************************
*
*  Syntax
*
*     W32	hmiINIGetHexIndex( BYTE bValue )
*
*  Description
*
*     Get the index of a hex character
*
*  Parameters
*
*        Type           Description
*        --------------------------
*        bValue         value to located
*
*  Return
*
*     index into table of hex value
*
****************************************************************************/
W32	hmiINIGetHexIndex( BYTE bValue )
	{
		W32	wIndex;

      // search
      for ( wIndex = 0; wIndex < 16; wIndex++ )
         if ( szHexNumbers[ wIndex ] == toupper( bValue ) )
            return( wIndex );

      // error return
      return( -1 );
	}
