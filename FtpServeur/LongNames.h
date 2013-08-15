
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

#ifndef LONG_NAME_H
#define LONG_NAME_H

#include <SdFat.h>

extern SdFat sd;
extern SdBaseFile* dirFile;

dir_t dirLfnFirst( char * lfn, char * sdn );
dir_t dirLfnNext( char * lfn );
boolean l2sName( char * sn, char * dn, char * ln, size_t lln = 0 );
boolean l2sName( char * sn, char * dn, char * ln, size_t lln );
boolean l2sPath( char * snd, char * lnd, size_t maxsnl );

#endif // LONG_NAME_H

