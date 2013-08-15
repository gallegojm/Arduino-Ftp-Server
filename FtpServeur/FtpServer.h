
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
#include <SdFat.h>

#define FTP_SERVER_VERSION "2013-08-14"

#define FTP_USER "arduino"
#define FTP_PASS "Due"

#define FTP_TIME_OUT 5       // Disconnect client after 5 minutes of inactivity
#define FTP_CMD_SIZE 256     // max size of a command
#define FTP_CWD_SIZE 256     // max size of a directory name
#define FTP_BUF_SIZE 512     // size of file buffer for read/write

class FtpServer
{
public:

  void init();
  void service();

private:

  void    clientConnected();
  void    disconnectClient();
  boolean userIdentity();
  boolean userPassword();
  boolean processCommand();
  boolean doRetrieve();
  boolean doStore();
  void    closeTransfer();
  int8_t  readChar();

  IPAddress dataIp;               // IP address of client for data
  EthernetClient client;
  EthernetClient data;
  uint16_t dataPort;
  uint8_t buf[ FTP_BUF_SIZE ];    // data buffer for transfers
  char cmdLine[ FTP_CMD_SIZE ];   // where to store incoming char from client
  char cwdLName[ FTP_CWD_SIZE ];  // long name for current directory
  char cwdSName[ FTP_CWD_SIZE ];  // short name for current directory
  char command[ 5 ];              // command sent by client
  char * parameters;              // point to begin of parameters sent by client
  uint16_t iCL;                   // pointer to cmdLine next incoming char
  int8_t cmdStatus,               // status of ftp command connexion
         transferStatus;          // status of ftp data transfer
  uint32_t millisTimeOut,         // disconnect after 5 min of inactivity
           millisEndConnection,   // 
           millisBeginTrans,      // store time of beginning of a transaction
           bytesTransfered;       //
  SdFile file;
};

#endif // FTP_SERVER_H


