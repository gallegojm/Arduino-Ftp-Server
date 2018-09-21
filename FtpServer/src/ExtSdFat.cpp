/*
 * Classes ExtSdFat and ExtDir by Jean-Michel Gallego
 * Copyright (c) 2018 by Jean-Michel Gallego
 *
 *   SdFat is an Arduino library written by William Greiman
 *    that provides read/write access to FAT16/FAT32
 *    file systems on SD/SDHC flash cards.
 *
 * You must install SdFat in order to use ExtSdFat and ExtDir
*/

#include "ExtSdFat.h"

/* ===========================================================

                    ExtFat functions

   =========================================================== */

bool ExtSdFat::timeStamp( char * path, uint16_t year, uint8_t month, uint8_t day,
                        uint8_t hour, uint8_t minute, uint8_t second )
{
  SdFile file;
  bool res;

  if( ! file.open( path, O_READ ))
    return false;
  res = file.timestamp( T_WRITE, year, month, day, hour, minute, second );
  file.close();
  return res;
}
                        
bool ExtSdFat::getFileModTime( char * path, uint16_t * pdate, uint16_t * ptime )
{
  SdFile file;
  dir_t d;
  bool res;

  if( ! file.open( path, O_READ ))
    return false;
  res = file.dirEntry( & d );
  if( res )
  {
    * pdate = d.lastWriteDate;
    * ptime = d.lastWriteTime;
  }
  file.close();
  return res;
}

/* ===========================================================

                    ExtDir functions

   =========================================================== */

// Open a directory
//   dirPath : absolute name of directory
// Return true if ok

bool ExtDir::openDir( char * dirPath )
{
  if( * dirPath != 0 )
    return open( dirPath );
  else
    return open( "/" );
}

// Read next directory entry
// Return false if end of directory is reached or an error had occurred

bool ExtDir::nextFile()
{
  dir_t d;

  if( ! curFile.openNext( this, O_READ ))
    return false;
  if( ! curFile.dirEntry( & d ))
    return false;
  curFile.getName( lfn, _MAX_LFN );
  isdir = curFile.isSubDir();
  filesize = curFile.fileSize();
  filelwd = d.lastWriteDate;
  filelwt = d.lastWriteTime;
  curFile.close();
  return true;
}

/* ===========================================================

                    ExtFile functions

   =========================================================== */

// Read a string from the file
//   str : read buffer
//   len : size of read buffer
// Return number of characters read or -1 if an error occurs

int16_t ExtFile::readString( char * str, int len )
{
  if( fgets( str, len ) < 0 )
    return -1;
  uint16_t lstr = strlen( str ) - 1;
  while( lstr > 0 && ( str[ lstr ] == '\n' || str[ lstr ] == '\r' ))
    str[ lstr -- ] = 0;
  return lstr;
}

// Read next literal integer from file
// Return value of integer

uint16_t ExtFile::readInt()
{
  uint16_t i = 0;
  char c;
  // skip characters they are not integer
  do
    c = read();
  while( c >= 0 && ! isdigit( c ));
  while( isdigit( c ))
  {
    i = 10 * i + c - '0';
    c = read();
  }
  return i;
}

// Read next literal hexadecinal integer from file
// Return value of integer

uint16_t ExtFile::readHex()
{
  uint16_t i = 0;
  char c;
  // skip characters they are not hexadecimal
  do
    c = read();
  while( c >= 0 && ! isxdigit( c ));
  while( isxdigit( c ))
  {
    i = 16 * i + c;
    if( isdigit( c ))
      i -= '0';
    else
    {
      i += 10;
      if( c < 'F' )
        i -= 'A';
      else
        i -= 'a';
    }
    c = read();
  }
  return i;
}

