
/*
 * FTP Serveur for Arduino Due and Ethernet shield (W5100) or WIZ820io (W5200)
 * Copyright (c) 2013 by Jean-Michel Gallego
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
 **                         FUNCTIONS FOR FTP SERVER                           **
 **                                                                            **
 *******************************************************************************/

#ifndef FTP_SERVER_H
#define FTP_SERVER_H

#include <Streaming.h>
#include <Ethernet.h>
#include "SdList.h"

// Uncomment to print debugging info to console attached to Arduino
#define FTP_DEBUG

#define FTP_SERVER_VERSION "FTP-2014-06-10"

#define FTP_USER "arduino"
#define FTP_PASS "Due"

#define FTP_CTRL_PORT 21         // Command port on wich server is listening
#define FTP_DATA_PORT_DFLT 20    // Default data port in active mode
#define FTP_DATA_PORT_PASV 55600 // Data port in passive mode

#define FTP_TIME_OUT 5           // Disconnect client after 5 minutes of inactivity
#define FTP_CMD_SIZE 256         // max size of a command
#define FTP_CWD_SIZE 256         // max size of a directory name
#define FTP_FIL_SIZE 128         // max size of a file name
#define FTP_BUF_SIZE 512         // size of file buffer for read/write

class FtpServer
{
public:
  void    init();
  void    service();

private:
  void    iniVariables();
  void    clientConnected();
  void    disconnectClient();
  boolean userIdentity();
  boolean userPassword();
  boolean processCommand();
  int     dataConnect();
  boolean doRetrieve();
  boolean doStore();
  void    closeTransfer();
  boolean makePathName( char * name, char * path, size_t maxpl );
  int8_t  readChar();

  IPAddress dataIp;               // IP address of client for data
  EthernetClient client;
  EthernetClient data;
  SdFile file;
  boolean dataPassiveConn;
  uint16_t dataPort;
  uint8_t buf[ FTP_BUF_SIZE ];    // data buffer for transfers
  char cmdLine[ FTP_CMD_SIZE ];   // where to store incoming char from client
  char cwdName[ FTP_CWD_SIZE ];   // name of current directory
  char cwdRNFR[ FTP_CWD_SIZE ];   // name of origin directory for Rename command
  char command[ 5 ];              // command sent by client
  char * parameters;              // point to begin of parameters sent by client
  uint16_t iCL;                   // pointer to cmdLine next incoming char
  int8_t cmdStatus,               // status of ftp command connexion
         transferStatus;          // status of ftp data transfer
  uint32_t millisTimeOut,         // disconnect after 5 min of inactivity
           millisEndConnection,   // 
           millisBeginTrans,      // store time of beginning of a transaction
           bytesTransfered;       //
};

#endif // FTP_SERVER_H


