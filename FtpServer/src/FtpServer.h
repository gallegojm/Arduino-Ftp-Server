/*
 * FTP Serveur for Arduino Due, Arduino MKR
 * and Ethernet shield W5100, W5200 or W5500
 * ( or for Esp8266 with external SD card or SpiFfs ) **
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

#define FTP_SERVER_VERSION "2020-12-08"

#include "FtpServerConfig.h"
#include <SPI.h>

#ifdef ESP8266
  #include <ESP8266WiFi.h>
  #include <WiFiClient.h>
#else
  #include <Ethernet.h>
#endif

#include <sdios.h>

#if FTP_FILESYST <= FTP_SDFAT2
  #include <SdFat.h>
  #define FTP_FS sd
  #define FTP_FILE SdFile
  #define FTP_DIR SdFile
  extern SdFat FTP_FS;
#elif FTP_FILESYST == FTP_SPIFM
  #include <SdFat.h>
  #include <Adafruit_SPIFlash.h>
  #define FTP_FS fatfs
  #define FTP_FILE File
  #define FTP_DIR File
  extern FatFileSystem FTP_FS;
  extern Adafruit_SPIFlash flash;
#elif FTP_FILESYST == FTP_FATFS
  #include <FatFs.h>
  #define FTP_FS sdff
  #define FTP_FILE FileFs
  #define FTP_DIR DirFs
  extern FatFsClass FTP_FS;
  #define O_READ     FA_READ
  #define O_WRITE    FA_WRITE
  #define O_RDWR     FA_READ | FA_WRITE
  #define O_CREAT    FA_CREATE_ALWAYS
  #define O_APPEND   FA_OPEN_APPEND
#endif

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

#define FTP_USER "arduino"        // Default user'name
#define FTP_PASS "test"           // Default password

#define FTP_CMD_PORT 21           // Command port on wich server is listening
#define FTP_DATA_PORT_DFLT 20     // Default data port in active mode
#define FTP_DATA_PORT_PASV 55600  // Data port in passive mode

#define FF_MAX_LFN 255            // max size of a long file name 
#define FTP_CMD_SIZE FF_MAX_LFN+8 // max size of a command
#define FTP_CWD_SIZE FF_MAX_LFN+8 // max size of a directory name
#define FTP_FIL_SIZE FF_MAX_LFN   // max size of a file name 
#define FTP_CRED_SIZE 16          // max size of username and password
#define FTP_NULLIP() IPAddress(0,0,0,0)

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

/*
class FtpFile : public SdFile
{
#if FTP_FILESYST == FTP_SPIFM
#endif
};
*/

class FtpServer
{
public:
  FtpServer( uint16_t _cmdPort = FTP_CMD_PORT, uint16_t _pasvPort = FTP_DATA_PORT_PASV );

  void    init( IPAddress _localIP = FTP_NULLIP() );
  void    credentials( const char * _user, const char * _pass );
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
  bool    openDir( FTP_DIR * pdir );
  bool    isDir( char * path );
  uint8_t getDateTime( char * dt, uint16_t * pyear, uint8_t * pmonth, uint8_t * pday,
                       uint8_t * phour, uint8_t * pminute, uint8_t * second );
  char *  makeDateTimeStr( char * tstr, uint16_t date, uint16_t time );
  bool    timeStamp( char * path, uint16_t year, uint8_t month, uint8_t day,
                     uint8_t hour, uint8_t minute, uint8_t second );
  bool    getFileModTime( char * path, uint16_t * pdate, uint16_t * ptime );
#if FTP_FILESYST != FTP_FATFS
  bool    getFileModTime( uint16_t * pdate, uint16_t * ptime );
#endif
  int8_t  readChar();

  bool     exists( const char * path ) { return FTP_FS.exists( path ); };
  bool     remove( const char * path ) { return FTP_FS.remove( path ); };
  bool     makeDir( const char * path ) { return FTP_FS.mkdir( path ); };
  bool     removeDir( const char * path ) { return FTP_FS.rmdir( path ); };
  bool     rename( const char * path, const char * newpath )
             { return FTP_FS.rename( path, newpath ); };
#if FTP_FILESYST == FTP_SDFAT1
  uint32_t capacity() { return FTP_FS.card()->cardSize() >> 1; };
  uint32_t free() { return FTP_FS.vol()->freeClusterCount() * 
                           FTP_FS.vol()->sectorsPerCluster() >> 1; };
#elif FTP_FILESYST == FTP_SDFAT2
  uint32_t capacity() { return FTP_FS.card()->sectorCount() >> 1; };
  uint32_t free() { return FTP_FS.vol()->freeClusterCount() * 
                           FTP_FS.vol()->sectorsPerCluster() >> 1; };
#elif FTP_FILESYST == FTP_SPIFM
  uint32_t capacity() { return flash.size() >> 10; };
  uint32_t free() { return 0; };    // TODO //
#elif FTP_FILESYST == FTP_FATFS
  uint32_t capacity() { return FTP_FS.capacity(); };
  uint32_t free() { return FTP_FS.free(); };
#endif
	bool    legalChar( char c ) // Return true if char c is allowed in a long file name
	{
		if( c == '"' || c == '*' || c == '?' || c == ':' || 
		    c == '<' || c == '>' || c == '|' )
		  return false;
#if FTP_FILESYST == FTP_FATFS
		return 0x1f < c && c < 0xff;
#else
		return 0x1f < c && c < 0x7f;
#endif
	}
  
  IPAddress   localIp;                // IP address of server as seen by clients
  IPAddress   dataIp;                 // IP address of client for data
  FTP_SERVER  ftpServer;
  FTP_SERVER  dataServer;
  FTP_CLIENT  client;
  FTP_CLIENT  data;
  
  FTP_FILE     file;
  FTP_DIR      dir;
  
  ftpCmd      cmdStage;               // stage of ftp command connexion
  ftpTransfer transferStage;          // stage of data connexion
  ftpDataConn dataConn;               // type of data connexion

  ArduinoOutStream FtpOutCli;
  ArduinoOutStream FtpOutData;
  
  uint8_t  __attribute__((packed, aligned(4))) // need to be aligned to 32bit for Esp8266 SPIClass::transferBytes()
           buf[ FTP_BUF_SIZE ];       // data buffer for transfers
  char     cmdLine[ FTP_CMD_SIZE ];   // where to store incoming char from client
  char     cwdName[ FTP_CWD_SIZE ];   // name of current directory
  char     rnfrName[ FTP_CWD_SIZE ];  // name of file for RNFR command
  char     user[ FTP_CRED_SIZE ];     // user name
  char     pass[ FTP_CRED_SIZE ];     // password
  char     command[ 5 ];              // command sent by client
  bool     rnfrCmd;                   // previous command was RNFR
  char *   parameter;                 // point to begin of parameters sent by client
  uint16_t cmdPort,
           pasvPort,
           dataPort;
  uint16_t iCL;                       // pointer to cmdLine next incoming char
  uint16_t nbMatch;

  uint32_t millisDelay,               //
           millisEndConnection,       // 
           millisBeginTrans,          // store time of beginning of a transaction
           bytesTransfered;           //
};

#endif // FTP_SERVER_H
