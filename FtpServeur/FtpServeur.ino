/*
 * This sketch demostrate the use of FTP server library
 * Copyright (c) 2014 by Jean-Michel Gallego
 */
 
#include <Streaming.h>
#include <SPI.h>
#include <Ethernet.h>
#include <SdFat.h>
#include "FtpServer.h"

// Define Chip Select for your SD card according to hardware 
#define CS_SDCARD 4  // SD card reader of Ehernet shield
// #define CS_SDCARD 9

SdFat sd;
FtpServer ftpSrv;

// Mac address of ethernet adapter
byte mac[] = { 0x90, 0xa2, 0xda, 0x00, 0x00, 0x00 };

// IP address of FTP server
IPAddress serverIp( 192, 168, 1, 111 );

/*******************************************************************************
**                                                                            **
**                               INITIALISATION                               **
**                                                                            **
*******************************************************************************/

void setup()
{
  Serial.begin(9600);

  // Initialize the SdCard.
  if( ! sd.begin( CS_SDCARD, SPI_HALF_SPEED ))
    sd.initErrorHalt();
  if( ! sd.chdir( "/" ))
    sd.errorHalt( "sd.chdir" );
  pinMode( CS_SDCARD, OUTPUT );
  digitalWrite( CS_SDCARD, HIGH );

  // Initialize the network
  Ethernet.begin( mac, serverIp );

  // Initialize the FTP server
  ftpSrv.init();
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

