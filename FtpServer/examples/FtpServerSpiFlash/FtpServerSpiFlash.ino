/*
 * **********************  FTP server library for Arduino **********************
 *                  Copyright (c) 2014-2020 by Jean-Michel Gallego
 * 
 * This sketch demonstrate the use of FTP server library with SPIFlash memories
 *                  
 *  Needs Adafruit SPIFlash library
 *    https://github.com/adafruit/Adafruit_SPIFlash
 *  Needs fork by AdaFruit of SdFat library from William Greiman
 *    https://github.com/adafruit/SdFat
 * 
 *  You have to modify some of the definitions
 *  In libraries/FtpServer/src/FtpServerConfig.h :
 *   - set #define FTP_FILESYST FTP_SPIFM
 *
 *  You must check than your memory chip is defined in file
 *    libraries/Adafruit_SPIFlash/src/flash_devices.h
 *   and is listed in possible_devices[] array in file
 *    libraries/Adafruit_SPIFlash/src/Adafruit_SPIFlashBase.cpp
 *  For example, for Winbond W25Q32FV chip, add in Adafruit_SPIFlashBase.cpp a line
 *    W25Q32FV,
 *
 * In FtpServerConfig.h most definitions are commented
 *   FTP_DEBUG if defined, print to the Ide serial monitor information for debugging.
 *   FTP_BUF_SIZE is the size of the file buffer for read and write operations.
 *   FTP_TIME_OUT and FTP_AUTH_TIME_OUT are expressed in seconds.
 */

#include <SPI.h>
#include <SdFat.h>
#include <Adafruit_SPIFlash.h>
#include <sdios.h>
#include <FtpServer.h>
#include <FreeStack.h>

// Define Chip Select for SPI flash memory
#define CS_SPIFLASH 4  // Chip Select for SPI Flash

// Assume Chip Select for Ethernet module is 5

// Define Reset pin for W5200 or W5500
// set to -1 for other ethernet chip or if Arduino reset board is used
// #define W5x00_RESET -1
#define W5x00_RESET 3

// Objects for SPI flash memory
Adafruit_FlashTransport_SPI flashTransport( 4, SPI );
Adafruit_SPIFlash flash( & flashTransport );
FatFileSystem fatfs;

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
  cout << F( "=== Test of FTP Server for SPIFlash memory " ) << endl;
  cout << F( "=== with Adafruit_SPIFlash and SdFat_Adafruit_Fork ") << SD_FAT_VERSION << endl;

  // If other chips are connected to SPI bus, set to high the pin connected
  // to their CS before initializing Flash memory
  pinMode( 5, OUTPUT ); 
  digitalWrite( 5, HIGH );

  // Initialize the SPI memory
  cout << F("Mount the Flash memory... ");
  if( ! flash.begin())
  {
    cout << F("Unable to initialize Flash chip") << endl;
    while( true ) ;
  }
  cout << F("ok") << endl;
  cout << F("Flash chip JEDEC ID: 0x") << hex << flash.getJEDECID() << dec << endl;
  if( ! fatfs.begin( & flash ))
  {
    cout << F("Unable to mount filesystem") << endl;
    while( true ) ;
  }
  cout << F("Mounted filesystem!") << endl;

  // Show capacity of SD card
  cout << F("Capacity of card:   ") << long( flash.size() >> 10 )  
       << F(" kBytes") << endl;

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
