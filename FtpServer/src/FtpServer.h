
/*
 * FTP Serveur for Arduino Due or Mega 2580 
 * and Ethernet shield W5100, W5200 or W5500
 * Copyright (c) 2014-2018 by Jean-Michel Gallego
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
#include <Ethernet.h>
#include "ExtStreaming.h"
#include "ExtSdFat.h"

// Uncomment to print debugging info to console attached to Arduino
#define FTP_DEBUG

#define FTP_SERVER_VERSION "FTP-2018-09-06"

#define FTP_USER "arduino"        // User'name
#define FTP_PASS "Due"            // His password

#define FTP_CTRL_PORT 21          // Command port on wich server is listening
#define FTP_DATA_PORT_DFLT 20     // Default data port in active mode
#define FTP_DATA_PORT_PASV 55600  // Data port in passive mode

#define FTP_TIME_OUT  5 * 60      // Disconnect client after 5 minutes of inactivity
#define FTP_AUTH_TIME_OUT 10      // Wait for authentication for 10 seconds
#define FTP_CMD_SIZE _MAX_LFN + 8 // max size of a command
#define FTP_CWD_SIZE _MAX_LFN + 8 // max size of a directory name
#define FTP_FIL_SIZE _MAX_LFN     // max size of a file name
#define FTP_BUF_SIZE 1024 // 512  // size of file buffer for read/write

enum ftpCmd { FTP_Stop = 0,       // In this stage, stop any connection
              FTP_Init,           //  initialize some variables
              FTP_Client,         //  wait for client connection
              FTP_User,           //  wait for user name
              FTP_Pass,           //  wait for user password
              FTP_Cmd };          //  answers to commands

enum ftpTransfer { FTP_Close = 0, // In this stage, close data channel
                   FTP_Retrieve,  //  retrieve file
                   FTP_Store };   //  store file

class FtpServer
{
public:
  void    init();
  void    service();

private:
  void    iniVariables();
  void    clientConnected();
  void    disconnectClient();
  boolean processCommand();
  boolean haveParameter();
  int     dataConnect( boolean out150 = true );
  boolean doRetrieve();
  boolean doStore();
  void    closeTransfer();
  void    abortTransfer();
  boolean makePath( char * fullName, char * param = NULL );
  boolean makeExistsPath( char * path, char * param = NULL );
  boolean openDir( ExtDir * pdir, char * sdir = NULL );
  uint8_t getDateTime( char * dt, uint16_t * pyear, uint8_t * pmonth, uint8_t * pday,
                       uint8_t * phour, uint8_t * pminute, uint8_t * second );
  char *  makeDateTimeStr( char * tstr, uint16_t date, uint16_t time );
  int8_t  readChar();
  
  IPAddress      dataIp;              // IP address of client for data
  EthernetClient client;
  EthernetClient data;
  
  SdFile   file;

  ftpCmd   cmdStage;                  // stage of ftp command connexion
  ftpTransfer transferStage;          // stage of data connexion
  
  boolean  dataPassiveConn;
  uint16_t dataPort;
  char     buf[ FTP_BUF_SIZE ];       // data buffer for transfers
  char     cmdLine[ FTP_CMD_SIZE ];   // where to store incoming char from client
  char     cwdName[ FTP_CWD_SIZE ];   // name of current directory
  char     command[ 5 ];              // command sent by client
  boolean  rnfrCmd;                   // previous command was RNFR
  char *   parameter;                 // point to begin of parameters sent by client
  uint16_t iCL;                       // pointer to cmdLine next incoming char
  uint32_t millisDelay,               //
           millisEndConnection,       // 
           millisBeginTrans,          // store time of beginning of a transaction
           bytesTransfered;           //
};

#endif // FTP_SERVER_H


