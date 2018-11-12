/*
 * This sketch demonstrate the use of FTP server library
 *  with an Esp8266 and an external SD memory card 
 *  or internal flash file system
 * Copyright (c) 2014-2018 by Jean-Michel Gallego
 *
 * To select between external SD memory card or internal flash file system,
 *   select in file FatLib.h one of the lines
 *      #define FAT_USE  FAT_FATFS
 *   or
 *      #define FAT_USE  FAT_SPIFFS
 *
 * You too may have to modify some of the definitions
 *
 * In ExtSdFat.h
 *   _MAX_LFN  is the longest size for a file name, including the complete path.
 *             It is set to 255 but can be reduced if memory usage is critical.
 *   
 * In FtpServer.h most definitions are commented
 *   FTP_DEBUG if defined, print to the Ide serial monitor information for debugging.
 *   FTP_USER  the user' name
 *   FTP_PASS  his password
 *   FTP_BUF_SIZE is the size of the file buffer for read and write operations.
 *                The value 1024 gives the best speed results.
 *                But it can be reduced if memory usage is critical.
 *   FTP_TIME_OUT and FTP_AUTH_TIME_OUT are expressed in seconds.
 *
 * In that file, you have to define the ssid and the password of your wifi AP
 */

#include <FtpServer.h>

FtpServer ftpSrv;

const char * ssid = "yourSsid";
const char * password = "yourPassword";

/*******************************************************************************
**                                                                            **
**                               INITIALISATION                               **
**                                                                            **
*******************************************************************************/

void setup()
{
  Serial.begin( 115200 );
  do
    delay( 10 );
  while( Serial.available() && Serial.read() >= 0 );
  Serial << eol << eol << F( "=== Test of FTP Server ===" ) << eol;

  // Initialize the SD card.
  Serial << F("Mount the file system with library ");
#if FAT_USE == FAT_FATFS
  Serial << F("FatFs ... ");
#elif FAT_USE == FAT_SPIFFS
  Serial << F("SpiFfs ... ");
#endif
  if( ! FAT_FS.begin())
  {
    Serial << F("Unable to mount file system") << eol;
    while( true )
      delay( 0 );
  }
  Serial << F("ok") << eol;

  // Initialize the wifi connection
  Serial << F("Initialize wifi connection ");
  WiFi.begin( ssid, password );
  
  // ... Give ESP 10 seconds to connect to station.
  uint32_t startTime = millis();
  while( WiFi.status() != WL_CONNECTED && millis() - startTime < 10000 )
  {
    Serial << ".";
    delay( 500 );
  }
  Serial << eol;

  // Check connection
  if( WiFi.status() == WL_CONNECTED )
    Serial << "WiFi connected to " << ssid << eol
           << "IP address: " << WiFi.localIP() << eol;

  // Initialize the FTP server
  ftpSrv.init();

  Serial << F("Free heap: ") << ESP.getFreeHeap() << eol;
}

/*******************************************************************************
**                                                                            **
**                                 MAIN LOOP                                  **
**                                                                            **
*******************************************************************************/

void loop()
{
  ftpSrv.service();
 
  // more processes... 
}
