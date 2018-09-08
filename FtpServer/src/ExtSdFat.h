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

#ifndef EXT_SDFAT_H
#define EXT_SDFAT_H

#include <SdFat.h>

#define _MAX_LFN 255

class ExtSdFat : public SdFat
{
public:
  int32_t  capacity() { return card()->cardSize() >> 11; };
  int32_t  free()     { return vol()->freeClusterCount() * vol()->blocksPerCluster() >> 11; };
  bool     timeStamp( char * path, uint16_t year, uint8_t month, uint8_t day,
                      uint8_t hour, uint8_t minute, uint8_t second );
  bool     getFileModTime( char * path, uint16_t * pdate, uint16_t * ptime );
};

extern ExtSdFat sd;

class ExtDir : public SdFile
{
public:
  bool     openDir( char * dirPath );
  bool     closeDir() { return close(); };
  bool     nextFile();
  bool     isDir()    { return isdir; };
  char *   fileName() { return lfn; };
  uint32_t fileSize() { return filesize; };
  uint16_t fileModDate() { return filelwd; };
  uint16_t fileModTime() { return filelwt; };

private:
  SdFile   curFile;
  char     lfn[ _MAX_LFN + 1 ];    // Buffer to store the LFN
  bool     isdir;
  uint32_t filesize;
  uint16_t filelwd;
  uint16_t filelwt;
};

#endif // EXT_SDFAT_H

