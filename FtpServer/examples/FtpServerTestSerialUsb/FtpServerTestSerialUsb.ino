/*
 * This sketch demonstrate the use of FTP server library
 * Copyright (c) 2014-2020 by Jean-Michel Gallego
 *
 * You may have to modify some of the definitions
 * Please read comments in example FtpServerTest
 *
 * If you need to be able to use a diferent serial port
 *  (Serial1, Serial2, SerialUSB, ... ) a simple way is
 *  to modify in FtpServer.h the line
 *    #define SerialFtp Serial
 *  and to use SerialFtp instead of Serial in your code
 *  as this example do.
*/

#include <FtpServer.h>
#include <FreeStack.h>

// Define Chip Select for your SD card according to hardware 
// #define CS_SDCARD 4  // SD card reader of Ehernet shield
#define CS_SDCARD 53 // Chip Select for SD card reader on Due

// Define Reset pin for W5200 or W5500
// set to -1 for other ethernet chip or if Arduino reset board is used
#define P_RESET -1
// #define P_RESET 8

FtpServer ftpSrv;

// Mac address of ethernet adapter
// byte mac[] = { 0x90, 0xa2, 0xda, 0x00, 0x00, 0x00 };
// byte mac[] = { 0x00, 0xaa, 0xbb, 0xcc, 0xde, 0xef };
byte mac[] = { 0xde, 0xad, 0xbe, 0xef, 0xfe, 0xef };

// IP address of FTP server
// if set to 0, use DHCP for the routeur to assign IP
// IPAddress serverIp( 192, 168, 1, 40 );
IPAddress serverIp( 0, 0, 0, 0 );

/*******************************************************************************
**                                                                            **
**                               INITIALISATION                               **
**                                                                            **
*******************************************************************************/

void setup()
{
  SerialFtp.begin( 115200 );
  delay( 4000 );
  SerialFtp << F( "=== Test of FTP Server ===" ) << eol;

  // If other chips are connected to SPI bus, set to high the pin connected to their CS
  // pinMode( 4, OUTPUT ); 
  // digitalWrite( 4, HIGH );

  // Initialize the SD card.
  SerialFtp << F("Mount the SD card with library ");
  #if FAT_USE == FAT_SDFAT
    SerialFtp << F("SdFat ... ");
  #else
    SerialFtp << F("FatFs ... ");
  #endif
  if( ! FAT_FS.begin( CS_SDCARD, SD_SCK_MHZ( 50 )))
  {
    SerialFtp << F("Unable to mount SD card") << eol;
    while( true ) ;
  }
  pinMode( CS_SDCARD, OUTPUT ); 
  digitalWrite( CS_SDCARD, HIGH );
  SerialFtp << F("ok") << eol;

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
  SerialFtp << F("Initialize ethernet module ... ");
  if( serverIp[0] != 0 )
    Ethernet.begin( mac, serverIp );
  else if( Ethernet.begin( mac ) == 0 )
  {
    SerialFtp << F("failed!") << eol;
    while( true ) ;
  }
  uint16_t wizModule[] = { 0, 5100, 5200, 5500 };
  SerialFtp << F("W") << wizModule[ Ethernet.hardwareStatus()] << F(" ok") << eol;
  SerialFtp << F("IP address of server: ") << Ethernet.localIP() << eol;

  // Initialize the FTP server
    ftpSrv.init();

  SerialFtp << F("Free stack: ") << FreeStack() << eol;
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
