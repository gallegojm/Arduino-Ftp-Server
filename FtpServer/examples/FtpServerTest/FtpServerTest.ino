/*
 * This sketch demonstrate the use of FTP server library
 * Copyright (c) 2014-2018 by Jean-Michel Gallego

 * You may have to modify some of the definitions
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
 */

#include <FtpServer.h>
#include <FreeStack.h>

// Define Chip Select for your SD card according to hardware 
// #define CS_SDCARD 4  // SD card reader of Ehernet shield
#define CS_SDCARD 53 // Chip Select for Arduino Due

// Define Reset pin for W5200 or W5500
// set to -1 for other ethernet chip or if Arduino reset board is used
// #define P_RESET -1
#define P_RESET 8

FtpServer ftpSrv;

// Mac address of ethernet adapter
// byte mac[] = { 0x90, 0xa2, 0xda, 0x00, 0x00, 0x00 };
byte mac[] = { 0x00, 0xaa, 0xbb, 0xcc, 0xde, 0xef };

// IP address of FTP server
// if set to 0, use DHCP for the routeur to assign IP
// IPAddress serverIp( 192, 168, 1, 100 );
IPAddress serverIp( 0, 0, 0, 0 );

/*******************************************************************************
**                                                                            **
**                               INITIALISATION                               **
**                                                                            **
*******************************************************************************/

void setup()
{
  Serial.begin( 9600 );
  Serial << F( "=== Test of FTP Server ===" ) << eol;

  // If other chips are connected to SPI bus, set to high the pin connected to their CS
  // pinMode( 10, OUTPUT ); 
  // digitalWrite( 10, HIGH );

  // Initialize the SD card.
  Serial << F("Mount the SD card with library SdFat ... ");
  if( ! sd.begin( CS_SDCARD, SD_SCK_MHZ( 50 )))
  {
    Serial << F("Unable to mount SD card") << eol;
    while( true ) ;
  }
  pinMode( CS_SDCARD, OUTPUT ); 
  digitalWrite( CS_SDCARD, HIGH );
  Serial << F("ok") << eol;

  // Send reset to Ethernet module
  if( P_RESET > -1 )
  {
    pinMode( P_RESET, OUTPUT );
    digitalWrite( P_RESET, LOW );
    delay( 200 );
    digitalWrite( P_RESET, HIGH );
    delay( 200 );
  }

  // Initialize the network
  Serial << F("Initialize ethernet module ... ");
  if( serverIp[0] != 0 )
    Ethernet.begin( mac, serverIp );
  else if( Ethernet.begin( mac ) == 0 )
  {
    Serial << F("failed!") << eol;
    while( true ) ;
  }
  uint16_t wizModule[] = { 0, 5100, 5200, 5500 };
  Serial << F("W") << wizModule[ Ethernet.hardwareStatus()] << F(" ok") << eol;
  serverIp = Ethernet.localIP();
  Serial << F("IP address of server: ") 
       << serverIp[0] << "." << serverIp[1] << "."
       << serverIp[2] << "." << serverIp[3] << eol;

  // Initialize the FTP server
  ftpSrv.init();

  Serial << F("Free stack: ") << FreeStack() << eol;
}

/*******************************************************************************
**                                                                            **
**                                 MAIN LOOP                                  **
**                                                                            **
*******************************************************************************/

void loop()
{
  ftpSrv.service();
 
  // more process... 
}

