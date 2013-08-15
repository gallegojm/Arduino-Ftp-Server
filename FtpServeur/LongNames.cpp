
/*
 * Extension for SdFat library to handle long names
 * Thanks to William Greiman (see http://forum.arduino.cc/index.php?topic=171663.0 )
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "LongNames.h"

// Search for the first file in a directory
//
// parameters:
//   char * lfn : where to store the 'long name' file
//                (must have a dim of at least 131 char)
//   char * sdn : the directory (short name) we want to scan
//
// return:
//   a dir_t object pointing to the founded file

dir_t dirLfnFirst( char * lfn, char * sdn )
{
  sd.chdir( sdn );
  dirFile = sd.vwd();
  dirFile->rewind();
  return dirLfnNext( lfn );
}

// Search for the next file
//
// parameters:
//   char * lfn : where to store the 'long name' file
//                (must have a dim of at least 131 char)
//
// return:
//   a dir_t object pointing to the founded file

dir_t dirLfnNext( char * lfn )
{
  uint8_t offset[] = {1, 3, 5, 7, 9, 14, 16, 18, 20, 22, 24, 28, 30};
  dir_t dir;
  uint8_t lfnIn = 130;
  uint8_t i;
  uint8_t ndir;
  uint8_t sum;
  uint8_t test;
  bool haveLong = false;

  while( dirFile->read( &dir, 32 ) == 32 )
  {
    if( DIR_IS_LONG_NAME( &dir ) )
    {
      if( ! haveLong )
      {
        if(( dir.name[0] & 0XE0 ) != 0X40 )
          continue;
        ndir = dir.name[0] & 0X1F;
        test = dir.creationTimeTenths;
        haveLong = true;
        lfnIn = 130;
        lfn[ lfnIn ] = 0;
      }
      else if( dir.name[0] != --ndir || test != dir.creationTimeTenths )
      {
        haveLong = false;
        continue;
      }
      char *p = (char*) & dir;
      if( lfnIn > 0 )
      {
        lfnIn -= 13;
        for( i = 0; i < 13; i++ )
          lfn[lfnIn + i] = p[offset[i]];
      }
    }
    else if( DIR_IS_FILE_OR_SUBDIR( &dir ) 
             && dir.name[0] != DIR_NAME_DELETED 
             && dir.name[0] != DIR_NAME_FREE
             ) // && dir.name[0] != '.' )
    {
      if( haveLong )
      {
        for( sum = i = 0; i < 11; i++ )
           sum = (((sum & 1) << 7) | ((sum & 0xfe) >> 1)) + dir.name[i];
        if( sum != test || ndir != 1 )
        haveLong = false;
      }
      if( haveLong )
      {
        for( i = 0; lfnIn + i <= 130 ; i++ )
          lfn[i] = lfn[lfnIn + i];
        return dir;
      }
      // else if( dir.reservedNT )
      //  return "Reserved NT";
      else
      {
        SdFile::dirName( dir, lfn );
        return dir;  
      }
    }
    else
    {
      if( dir.name[0] == DIR_NAME_FREE )
        break;
      haveLong = false;
    }
  }
  lfn[ 0 ] = 0;
  return dir;
}

// Convert a 'long name' file to a 'short name' file
//
// parameters:
//   char * sn : where to store the 'short name' file
//   char * dn : the directory (short name) where is the file
//   char * ln : the 'long name' file we want to convert
//   size_t lln : the length of the long name file. If 0 (default),
//                calculated with strlen( ln )
//
// return:
//   true, if convertion is done

boolean l2sName( char * sn, char * dn, char * ln, size_t lln )
{
  char ln0[131];
  ln0[0] = 0;
  dir_t dir = dirLfnFirst( ln0, dn );
  if( lln == 0 )
    lln = strlen( ln );
  while(( strlen( ln0 ) > 0 ) && ( strncmp( ln0, ln, lln ) != 0 ))
{
    dir = dirLfnNext( ln0 );
}
  SdFile::dirName( dir, sn );
  return strlen( sn ) != 0;
}

// Convert a 'long name' path to a 'short name' path
//
// parameters:
//   char * snd : where to store the 'short name' path
//   char * lnd : the 'long name' path we want to convert
//   size_t maxsnl : the length of snd string
//
// return:
//   true, if convertion is done

boolean l2sPath( char * snd, char * lnd, size_t maxsnl )
{
  char * dir0 = lnd + 1;
  char * dir1;
  char * snd1;
  
  strcpy( snd, "/" );
  while( strlen( dir0 ) > 1 )
  {
    dir1 = strchr( dir0, '/' );
    if( dir1 == 0 )
      return true;
    if( maxsnl <= strlen( snd ) + 8 )
      return false;
    snd1 = strrchr( snd, 0 );
//    snd1 = snd + strlen( snd );
    if( ! l2sName( snd1 , snd, dir0, dir1 - dir0 ))
      return false;
    strcat( snd, "/" );
    dir0 = dir1 + 1;
  }
  return true;
}

