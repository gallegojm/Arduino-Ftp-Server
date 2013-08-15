
/*
 * FTP Serveur for Arduino Due and Ethernet shield (W5100) or WIZ820io (W5200)
 * Copyright (c) 2013 by Jean-Michel Gallego
 * 
 * Use Streaming.h from Mial Hart
 *
 * Use SdFat.h from William Greiman
 *   with extension for long names (see http://forum.arduino.cc/index.php?topic=171663.0 )
 *
 * Use Ethernet library with somes modifications:
 *   modification for WIZ820io (see http://forum.arduino.cc/index.php?topic=139147.0 
 *     and https://github.com/jbkim/W5200-Arduino-Ethernet-library )
 *   need to add the function EthernetClient EthernetServer::connected()
 *     (see http://forum.arduino.cc/index.php?topic=169165.15 
 *      and http://forum.arduino.cc/index.php?topic=182354.0 )
 *     In EthernetServer.h add:
 *           EthernetClient connected();
 *     In EthernetServer.h add:
 *           EthernetClient EthernetServer::connected()
 *           {
 *             accept();
 *             for( int sock = 0; sock < MAX_SOCK_NUM; sock++ )
 *               if( EthernetClass::_server_port[sock] == _port )
 *               {
 *                 EthernetClient client(sock);
 *                 if( client.status() == SnSR::ESTABLISHED ||
 *                     client.status() == SnSR::CLOSE_WAIT )
 *                   return client;
 *               }
 *             return EthernetClient(MAX_SOCK_NUM);
 *           }
 * 
 * Commands implemented: 
 *   USER, PASS
 *   ABOR, QUIT
 *   CDUP, CWD, PWD
 *   FEAT, MLSD, SIZE
 *   NLST, LIST (poorly)
 *   MODE, STRU, TYPE
 *   PORT
 *   RETR, STOR (only 8.3 files name)
 *
 * Tested with those clients:
 *   FTP Rush : ok
 *   Filezilla: problem whit RETR
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

#include <Streaming.h>
#include <SPI.h>
#include <Ethernet.h>
#include <SdFat.h>
#include "LongNames.h"
#include "FtpServer.h"

// Define Chip Select for your SD card according to hardware 
#define CS_SDCARD 4  // SD card reader of Ehernet shield
// #define CS_SDCARD 9

SdFat sd;
SdBaseFile* dirFile;
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
  Serial << "Initiating Ftp Server on Arduino" << endl;

  // Initialize the SdCard.
  if( ! sd.begin( CS_SDCARD, SPI_HALF_SPEED ))
    sd.initErrorHalt();
  if( ! sd.chdir( "/" ))
    sd.errorHalt( "sd.chdir" );
  pinMode( CS_SDCARD, OUTPUT );
  digitalWrite( CS_SDCARD, HIGH );

  // Initialize the network
  Ethernet.begin( mac, serverIp );

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

