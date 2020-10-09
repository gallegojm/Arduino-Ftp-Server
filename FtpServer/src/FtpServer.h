/*
 * FTP Serveur for Arduino Due or Mega 2580 
 * and Ethernet shield W5100, W5200 or W5500
 * or for Esp8266 with external SD card or SpiFfs
 * Copyright (c) 2014-2020 by Jean-Michel Gallego
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

/*******************************************************************************
 **                                                                            **
 **                       DEFINITIONS FOR FTP SERVER                           **
 **                                                                            **
 *******************************************************************************/

#ifndef FTP_SERVER_H
#define FTP_SERVER_H

#include <SPI.h>
#ifdef ESP8266
  #include <ESP8266WiFi.h>
  #include <WiFiClient.h>
#else
  #include <Ethernet.h>
#endif
#include "ExtStreaming.h"
#include <FatLib.h>

// Uncomment to print debugging info to console attached to Arduino
#define FTP_DEBUG
// Uncomment to print additional info
//#define FTP_DEBUG1

#define FTP_SERVER_VERSION "2020-10-08"

#define FTP_USER "arduino"        // User'name
#define FTP_PASS "test"           // His password

#define FTP_CTRL_PORT 21          // Command port on wich server is listening
#define FTP_DATA_PORT_DFLT 20     // Default data port in active mode
#define FTP_DATA_PORT_PASV 55600  // Data port in passive mode

#define FTP_TIME_OUT  5 * 60      // Disconnect client after 5 minutes of inactivity
#define FTP_AUTH_TIME_OUT 10      // Wait for authentication for 10 seconds
#define FTP_CMD_SIZE FF_MAX_LFN+8 // max size of a command
#define FTP_CWD_SIZE FF_MAX_LFN+8 // max size of a directory name
#define FTP_FIL_SIZE FF_MAX_LFN   // max size of a file name 
#define FTP_CRED_SIZE 16          // max size of username and password
#define FTP_BUF_SIZE 1024 // 512  // size of file buffer for read/write
#define FTP_NULLIP() IPAddress(0,0,0,0)

#ifdef ESP8266
  #define FTP_SERVER WiFiServer
  #define FTP_CLIENT WiFiClient
  #define FTP_LOCALIP() WiFi.localIP()
  #define CommandIs( a ) (command != NULL && ! strcmp_P( command, PSTR( a )))
  #define ParameterIs( a ) ( parameter != NULL && ! strcmp_P( parameter, PSTR( a )))
#else
  #define FTP_SERVER EthernetServer
  #define FTP_CLIENT EthernetClient
  #define FTP_LOCALIP() Ethernet.localIP()
  #define CommandIs( a ) ( ! strcmp_PF( command, PSTR( a )))
  #define ParameterIs( a ) ( ! strcmp_PF( parameter, PSTR( a )))
#endif

enum ftpCmd { FTP_Stop = 0,       //  In this stage, stop any connection
              FTP_Init,           //  initialize some variables
              FTP_Client,         //  wait for client connection
              FTP_User,           //  wait for user name
              FTP_Pass,           //  wait for user password
              FTP_Cmd };          //  answers to commands

enum ftpTransfer { FTP_Close = 0, // In this stage, close data channel
                   FTP_Retrieve,  //  retrieve file
                   FTP_Store,     //  store file
                   FTP_List,      //  list of files
                   FTP_Nlst,      //  list of name of files
                   FTP_Mlsd };    //  listing for machine processing

enum ftpDataConn { FTP_NoConn = 0,// No data connexion
                   FTP_Pasive,    // Pasive type
                   FTP_Active };  // Active type

class FtpServer
{
public:
  void    init( IPAddress _localIP = FTP_NULLIP() );
  void    credentials( char * _user, char * _pass );
  uint8_t service();

private:
  void    iniVariables();
  void    clientConnected();
  void    disconnectClient();
  bool    processCommand();
  bool    haveParameter();
  int     dataConnect( bool out150 = true );
  bool    dataConnected();
  bool    doRetrieve();
  bool    doStore();
  bool    doList();
  bool    doMlsd();
  void    closeTransfer();
  void    abortTransfer();
  bool    makePath( char * fullName, char * param = NULL );
  bool    makeExistsPath( char * path, char * param = NULL );
  bool    openDir( FAT_DIR * pdir, char * sdir = NULL );
  uint8_t getDateTime( char * dt, uint16_t * pyear, uint8_t * pmonth, uint8_t * pday,
                       uint8_t * phour, uint8_t * pminute, uint8_t * second );
  char *  makeDateTimeStr( char * tstr, uint16_t date, uint16_t time );
  int8_t  readChar();
  
  IPAddress   localIp;                // IP address of server as seen by clients
  IPAddress   dataIp;                 // IP address of client for data
  FTP_CLIENT  client;
  FTP_CLIENT  data;
  
  FAT_FILE file;
  FAT_DIR  dir;
  
  ftpCmd   cmdStage;                  // stage of ftp command connexion
  ftpTransfer transferStage;          // stage of data connexion
  ftpDataConn dataConn;               // type of data connexion
  
  uint8_t  __attribute__((packed, aligned(4))) // need to be aligned to 32bit for Esp8266 SPIClass::transferBytes()
           buf[ FTP_BUF_SIZE ];       // data buffer for transfers
  char     cmdLine[ FTP_CMD_SIZE ];   // where to store incoming char from client
  char     cwdName[ FTP_CWD_SIZE ];   // name of current directory
  char     rnfrName[ FTP_CWD_SIZE ];  // name of file for RNFR command
  char     user[ FTP_CRED_SIZE ];     // user name
  char     pass[ FTP_CRED_SIZE ];     // password
  char     command[ 5 ];              // command sent by client
  bool     rnfrCmd;                   // previous command was RNFR
  bool     sameSubnet;
  char *   parameter;                 // point to begin of parameters sent by client
  uint16_t dataPort;
  uint16_t iCL;                       // pointer to cmdLine next incoming char
  uint16_t nbMatch;

  uint32_t millisDelay,               //
           millisEndConnection,       // 
           millisBeginTrans,          // store time of beginning of a transaction
           bytesTransfered;           //
};

#endif // FTP_SERVER_H
