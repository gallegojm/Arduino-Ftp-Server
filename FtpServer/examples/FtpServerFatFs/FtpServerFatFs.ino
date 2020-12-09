/*
 * **********************  FTP server library for Arduino **********************
 *                  Copyright (c) 2014-2020 by Jean-Michel Gallego
 * 
 * This sketch demonstrate the use of FTP server library
 *   with FatFs library adapted from ChaN library
 *
 * Needs FatFs library 
 *   https://github.com/gallegojm/Arduino-FatFs
 * Needs SdFat library from William Greiman for low level device control (version 1.1.4)
 *   https://github.com/greiman/SdFat/releases/tag/1.1.4
 *
 *  You have to modify some of the definitions
 *  In libraries/FtpServer/src/FtpServerConfig.h :
 *   - set #define FTP_FILESYST FTP_FATFS
 *   
 * In FtpServerConfig.h most definitions are commented
 *   FTP_DEBUG if defined, print to the Ide serial monitor information for debugging.
 *   FTP_BUF_SIZE is the size of the file buffer for read and write operations.
 *   FTP_TIME_OUT and FTP_AUTH_TIME_OUT are expressed in seconds.
 */

#include <SPI.h>
#include <FatFs.h>
#include <sdios.h>
#include <FtpServer.h>
#include <FreeStack.h>

// Define Chip Select for your SD card according to hardware 
// #define CS_SDCARD 4  // SD card reader of Ehernet shield
#define CS_SDCARD 53 // Chip Select for SD card reader on Due

// Define Reset pin for W5200 or W5500
// set to -1 for other ethernet chip or if Arduino reset board is used
// #define W5x00_RESET -1
#define W5x00_RESET 8

// Object for File system
FatFsClass sdff;

// Object for FtpServer
//  Command port and Data port in passive mode can be defined here
// FtpServer ftpSrv( 221, 25000 );
// FtpServer ftpSrv( 421 ); // Default data port in passive mode is 55600
FtpServer ftpSrv; // Default command port is 21 ( !! without parenthesis !! )

// Mac address of ethernet adapter
// byte mac[] = { 0x90, 0xa2, 0xda, 0x00, 0x00, 0x00 };
byte mac[] = { 0x00, 0xaa, 0xbb, 0xcc, 0xde, 0xef };

// IP address of FTP server
// if set to 0, use DHCP for the routeur to assign IP
// IPAddress serverIp( 192, 168, 1, 40 );
IPAddress serverIp( 0, 0, 0, 0 );

// External IP address of FTP server
// In passive mode, when accessing the serveur from outside his subnet, it can be
//  necessary with some clients to reply them with the server's external ip address
// IPAddress externalIp( 192, 168, 1, 2 );

ArduinoOutStream cout( Serial );

/*******************************************************************************
**                                                                            **
**                               INITIALISATION                               **
**                                                                            **
*******************************************************************************/

void setup()
{
  Serial.begin( 115200 );
  while( ! Serial )
    delay(100);
  cout << endl << F( "=== Test of FTP Server with FatFs file system ===" ) << endl;

  // If other chips are connected to SPI bus, set to high the pin connected
  // to their CS before initializing Flash memory
  pinMode( 4, OUTPUT ); 
  digitalWrite( 4, HIGH );

  // Mount the SD card memory
  cout << F("Mount the SD card memory... ");
  if( ! sdff.begin( CS_SDCARD, SD_SCK_MHZ(50)))
  {
    cout << F("Unable to mount SD card") << endl;
    while( true ) ;
  }
  pinMode( CS_SDCARD, OUTPUT ); 
  digitalWrite( CS_SDCARD, HIGH );
  cout << F("ok") << endl;

  // Show capacity and free space of SD card
  Serial.print( "Capacity of card:   " );
  Serial.print( sdff.capacity());
  Serial.println( " kBytes" );
  Serial.print( "Free space on card: " );
  Serial.print( sdff.free());
  Serial.println( " kBytes" );

  // Send reset to Ethernet module
  if( W5x00_RESET > -1 )
  {
    pinMode( W5x00_RESET, OUTPUT );
    digitalWrite( W5x00_RESET, LOW );
    delay( 200 );
    digitalWrite( W5x00_RESET, HIGH );
    delay( 200 );
  }

  // Initialize the network
  cout << F("Initialize ethernet module ... ");
  if( serverIp[0] != 0 )
    Ethernet.begin( mac, serverIp );
  else if( Ethernet.begin( mac ) == 0 )
  {
    cout << F("failed!") << endl;
    while( true ) ;
  }
  uint16_t wizModule[] = { 0, 5100, 5200, 5500 };
  cout << F("W") << wizModule[ Ethernet.hardwareStatus()] << F(" ok") << endl;
  serverIp = Ethernet.localIP();
  cout << F("IP address of server: ")
       << int( serverIp[0]) << "." << int( serverIp[1]) << "."
       << int( serverIp[2]) << "." << int( serverIp[3]) << endl;

  // Initialize the FTP server
  ftpSrv.init();
  // ftpSrv.init( externalIp );
  // ftpSrv.init( IPAddress( 11, 22, 33, 44 ));

  // Default username and password are set to 'arduino' and 'test'
  //  but can then be changed by calling ftpSrv.credentials()
  // ftpSrv.credentials( "myname", "123" );

  cout << F("Free stack: ") << FreeStack() << endl;
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
