/*
 * FTP Serveur for Arduino Due and Ethernet shield (W5100) or WIZ820io (W5200)
 * Copyright (c) 2014-2015 by Jean-Michel Gallego
 *
 * Please read file ReadMe.txt for instructions 
 * 
 * Use Streaming.h from Mial Hart
 *
 * Use FatFs library from ChaN
 *   (see http://elm-chan.org/fsw/ff/00index_e.html )
 * or SdFat library from William Greiman
 *   (see https://github.com/greiman/SdFat )
 *
 * Use FatLib to select between FatFs and SdFat
 *
 * Use Ethernet library with some modifications:
 *   modification for WIZ820io (see http://forum.arduino.cc/index.php?topic=139147.0 
 *     and https://github.com/jbkim/W5200-Arduino-Ethernet-library )
 *   need to add the function EthernetClient EthernetServer::connected()
 *     (see http://forum.arduino.cc/index.php?topic=169165.15 
 *      and http://forum.arduino.cc/index.php?topic=182354.0 )
 *     In EthernetServer.h add:
 *           EthernetClient connected();
 *     In EthernetServer.cpp add:
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
 *   CDUP, CWD, QUIT
 *   MODE, STRU, TYPE
 *   PASV, PORT
 *   ABOR
 *   DELE
 *   LIST, MLSD, NLST
 *   NOOP, PWD
 *   RETR, STOR
 *   MKD,  RMD
 *   RNTO, RNFR
 *   MDTM
 *   FEAT, SIZE
 *   SITE FREE
 *
 * Tested with those clients:
 *   under Windows:
 *     FTP Rush : ok
 *     Filezilla : ok
 *   under Ubuntu:
 *     gFTP : ok
 *     Filezilla : ok
 *   with a second Arduino and sketch of SurferTim at
 *     http://playground.arduino.cc/Code/FTP
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

#include "FtpServer.h"

EthernetServer ftpServer( FTP_CTRL_PORT );
EthernetServer dataServer( FTP_DATA_PORT_PASV );

void FtpServer::init()
{
  // Tells the ftp server to begin listening for incoming connection
  ftpServer.begin();
  dataServer.begin();
  millisTimeOut = ( uint32_t ) FTP_TIME_OUT * 60 * 1000;
  millisDelay = 0;
  cmdStatus = 0;
  iniVariables();
}

void FtpServer::iniVariables()
{
  // Default for data port
  dataPort = FTP_DATA_PORT_DFLT;
  
  // Default Data connection is Active
  dataPassiveConn = false;
  
  // Set the root directory
  strcpy( cwdName, "/" );

  rnfrCmd = false;
  transferStatus = 0;
}

void FtpServer::service()
{
  if((int32_t) ( millisDelay - millis() ) > 0 )
    return;

  if( cmdStatus == 0 )
  {
    if( client.connected())
      disconnectClient();
    cmdStatus = 1;
  }
  else if( cmdStatus == 1 )         // Ftp server waiting for connection
  {
    abortTransfer();
    iniVariables();
    #ifdef FTP_DEBUG
      Serial << "Ftp server waiting for connection on port " << FTP_CTRL_PORT << endl;
    #endif
    cmdStatus = 2;
  }
  else if( cmdStatus == 2 )         // Ftp server idle
  {
    client = ftpServer.connected();
    if( client > 0 )                // A client connected
    {
      clientConnected();      
      millisEndConnection = millis() + 10 * 1000 ; // wait client id during 10 s.
      cmdStatus = 3;
    }
  }
  else if( readChar() > 0 )         // got response
  {
    if( cmdStatus == 3 )            // Ftp server waiting for user identity
      if( userIdentity() )
        cmdStatus = 4;
      else
        cmdStatus = 0;
    else if( cmdStatus == 4 )       // Ftp server waiting for user registration
      if( userPassword() )
      {
        cmdStatus = 5;
        millisEndConnection = millis() + millisTimeOut;
      }
      else
        cmdStatus = 0;
    else if( cmdStatus == 5 )       // Ftp server waiting for user command
      if( ! processCommand())
        cmdStatus = 0;
      else
        millisEndConnection = millis() + millisTimeOut;
  }
  else if( ! client.connected() )
    cmdStatus = 1;

  if( transferStatus == 1 )         // Retrieve data
  {
    if( ! doRetrieve())
      transferStatus = 0;
  }
  else if( transferStatus == 2 )    // Store data
  {
    if( ! doStore())
      transferStatus = 0;
  }
  else if( cmdStatus > 2 && ! ((int32_t) ( millisEndConnection - millis() ) > 0 ))
  {
    client << "530 Timeout\r\n";
    millisDelay = millis() + 200;    // delay of 200 ms
    cmdStatus = 0;
  }
}

void FtpServer::clientConnected()
{
  #ifdef FTP_DEBUG
    Serial << "Client connected!" << endl;
  #endif
  client << "220--- Welcome to FTP for Arduino ---\r\n";
  client << "220---   By Jean-Michel Gallego   ---\r\n";
  client << "220 --   Version " << FTP_SERVER_VERSION << "   --\r\n";
  iCL = 0;
}

void FtpServer::disconnectClient()
{
  #ifdef FTP_DEBUG
    Serial << " Disconnecting client" << endl;
  #endif
  abortTransfer();
  client << "221 Goodbye\r\n";
  client.stop();
}

boolean FtpServer::userIdentity()
{
  if( strcmp( command, "USER" ))
    client << "500 Syntax error\r\n";
  if( strcmp( parameters, FTP_USER ))
    client << "530 \r\n";
  else
  {
    client << "331 OK. Password required\r\n";
    strcpy( cwdName, "/" );
    return true;
  }
  millisDelay = millis() + 100;  // delay of 100 ms
  return false;
}

boolean FtpServer::userPassword()
{
  if( strcmp( command, "PASS" ))
    client << "500 Syntax error\r\n";
  else if( strcmp( parameters, FTP_PASS ))
    client << "530 \r\n";
  else
  {
    #ifdef FTP_DEBUG
      Serial << "OK. Waiting for commands." << endl;
    #endif
    client << "230 OK.\r\n";
    return true;
  }
  millisDelay = millis() + 100;  // delay of 100 ms
  return false;
}

boolean FtpServer::processCommand()
{
  ///////////////////////////////////////
  //                                   //
  //      ACCESS CONTROL COMMANDS      //
  //                                   //
  ///////////////////////////////////////

  //
  //  CDUP - Change to Parent Directory 
  //
  if( ! strcmp( command, "CDUP" ))
  {
    boolean ok = false;
    
    if( strlen( cwdName ) > 1 )            // do nothing if cwdName is root
    {
      // if cwdName ends with '/', remove it (must not append)
      if( cwdName[ strlen( cwdName ) - 1 ] == '/' )
        cwdName[ strlen( cwdName ) - 1 ] = 0;
      // search last '/'
      char * pSep = strrchr( cwdName, '/' );
      ok = pSep > cwdName;
      // if found, ends the string on its position
      if( ok )
      {
        * pSep = 0;
        ok = FAT.exists( cwdName );
      }
    }
    // if an error appends, move to root
    if( ! ok )
      strcpy( cwdName, "/" );
    client << "200 Ok. Current directory is " << cwdName << "\r\n";
  }
  //
  //  CWD - Change Working Directory
  //
  else if( ! strcmp( command, "CWD" ))
  {
    char path[ FTP_CWD_SIZE ];
    if( strcmp( parameters, "." ) == 0 )  // 'CWD .' is the same as PWD command
      client << "257 \"" << cwdName << "\" is your current directory\r\n";
    else if( makePath( path ))
    {
      if( ! FAT.exists( path ))
        client << "550 Can't change directory to " << parameters << "\r\n";
      else
      {
        strcpy( cwdName, path );
        client << "250 Ok. Current directory is " << cwdName << "\r\n";
      }
    }
  }
  //
  //  PWD - Print Directory
  //
  else if( ! strcmp( command, "PWD" ))
    client << "257 \"" << cwdName << "\" is your current directory\r\n";
  //
  //  QUIT
  //
  else if( ! strcmp( command, "QUIT" ))
  {
    disconnectClient();
    return false;
  }

  ///////////////////////////////////////
  //                                   //
  //    TRANSFER PARAMETER COMMANDS    //
  //                                   //
  ///////////////////////////////////////

  //
  //  MODE - Transfer Mode 
  //
  else if( ! strcmp( command, "MODE" ))
  {
    if( ! strcmp( parameters, "S" ))
      client << "200 S Ok\r\n";
    // else if( ! strcmp( parameters, "B" ))
    //  client << "200 B Ok\r\n";
    else
      client << "504 Only S(tream) is suported\r\n";
  }
  //
  //  PASV - Passive Connection management
  //
  else if( ! strcmp( command, "PASV" ))
  {
    data.stop();
    dataServer.begin();
    dataIp = Ethernet.localIP();
    dataPort = FTP_DATA_PORT_PASV;
    //data.connect( dataIp, dataPort );
    //data = dataServer.available();
    #ifdef FTP_DEBUG
      Serial << "Connection management set to passive" << endl;
      Serial << "Data port set to " << dataPort << endl;
    #endif
    client << "227 Entering Passive Mode ("
           << dataIp[0] << "," << dataIp[1] << "," << dataIp[2] << "," << dataIp[3]
           << "," << ( dataPort >> 8 ) << "," << ( dataPort & 255 )
           << ").\r\n";
    dataPassiveConn = true;
  }
  //
  //  PORT - Data Port
  //
  else if( ! strcmp( command, "PORT" ))
  {
    data.stop();
    // get IP of data client
    dataIp[ 0 ] = atoi( parameters );
    char * p = strchr( parameters, ',' );
    for( uint8_t i = 1; i < 4; i ++ )
    {
      dataIp[ i ] = atoi( ++ p );
      p = strchr( p, ',' );
    }
    // get port of data client
    dataPort = 256 * atoi( ++ p );
    p = strchr( p, ',' );
    dataPort += atoi( ++ p );
    if( p == NULL )
      client << "501 Can't interpret parameters\r\n";
    else
    {
      #ifdef FTP_DEBUG
        Serial << "Data IP set to " << dataIp[0] << ":" << dataIp[1]
               << ":" << dataIp[2] << ":" << dataIp[3] << endl;
        Serial << "Data port set to " << dataPort << endl;
      #endif
      client << "200 PORT command successful\r\n";
      dataPassiveConn = false;
    }
  }
  //
  //  STRU - File Structure
  //
  else if( ! strcmp( command, "STRU" ))
  {
    if( ! strcmp( parameters, "F" ))
      client << "200 F Ok\r\n";
    // else if( ! strcmp( parameters, "R" ))
    //  client << "200 B Ok\r\n";
    else
      client << "504 Only F(ile) is suported\r\n";
  }
  //
  //  TYPE - Data Type
  //
  else if( ! strcmp( command, "TYPE" ))
  {
    if( ! strcmp( parameters, "A" ))
      client << "200 TYPE is now ASII\r\n";
    else if( ! strcmp( parameters, "I" ))
      client << "200 TYPE is now 8-bit binary\r\n";
    else
      client << "504 Unknow TYPE\r\n";
  }

  ///////////////////////////////////////
  //                                   //
  //        FTP SERVICE COMMANDS       //
  //                                   //
  ///////////////////////////////////////

  //
  //  ABOR - Abort
  //
  else if( ! strcmp( command, "ABOR" ))
  {
    abortTransfer();
    client << "226 Data connection closed" << "\r\n";
  }
  //
  //  DELE - Delete a File 
  //
  else if( ! strcmp( command, "DELE" ))
  {
    char path[ FTP_CWD_SIZE ];
    if( strlen( parameters ) == 0 )
      client << "501 No file name\r\n";
    else if( makePath( path ))
    {
      if( ! FAT.exists( path ))
        client << "550 File " << parameters << " not found\r\n";
      else
      {
        if( FAT.remove( path ))
          client << "250 Deleted " << parameters << "\r\n";
        else
          client << "450 Can't delete " << parameters << "\r\n";
      }
    }
  }
  //
  //  LIST - List 
  //
  else if( ! strcmp( command, "LIST" ))
  {
    if( ! dataConnect())
      client << "425 No data connection\r\n";
    else
    {
      client << "150 Accepted data connection\r\n";
      uint16_t nm = 0;
      FAT_DIR dir;
      if( ! dir.openDir( cwdName ))
        client << "550 Can't open directory " << cwdName << "\r\n";
      else
      {
        while( dir.nextFile())
        {
          if( dir.isDir() )
            data << "+/";
          else
            data << "+r,s" << dir.fileSize();
          data << ",\t" << dir.fileName() << "\r\n";
          nm ++;
        }
        client << "226 " << nm << " matches total\r\n";
      }
      data.stop();
    }
  }
  //
  //  MLSD - Listing for Machine Processing (see RFC 3659)
  //
  else if( ! strcmp( command, "MLSD" ))
  {
    if( ! dataConnect())
      client << "425 No data connection\r\n";
    else
    {
      client << "150 Accepted data connection\r\n";
      uint16_t nm = 0;
      FAT_DIR dir;
      char dtStr[ 15 ];
      if( ! dir.openDir( cwdName ))
        client << "550 Can't open directory " << parameters << "\r\n";
      else
      {
        while( dir.nextFile())
        {
          data << "Type=" << ( dir.isDir() ? "dir" : "file" ) << ";"
               << "Size=" << dir.fileSize() << ";"
               << "Modify=" << makeDateTimeStr( dtStr, dir.fileModDate(), dir.fileModTime()) << "; " 
               << dir.fileName() << "\r\n";
          nm ++;
        }
        client << "226-options: -a -l\r\n";
        client << "226 " << nm << " matches total\r\n";
      }
      data.stop();
    }
  }
  //
  //  NLST - Name List 
  //
  else if( ! strcmp( command, "NLST" ))
  {
    if( ! dataConnect())
      client << "425 No data connection\r\n";
    else
    {
      client << "150 Accepted data connection\r\n";
      uint16_t nm = 0;
      FAT_DIR dir;
      if( ! dir.openDir( cwdName ))
        client << "550 Can't open directory " << parameters << "\r\n";
      else
      {
        while( dir.nextFile())
        {
          data << dir.fileName() << "\r\n";
          nm ++;
        }
        client << "226 " << nm << " matches total\r\n";
      }
      data.stop();
    }
  }
  //
  //  NOOP
  //
  else if( ! strcmp( command, "NOOP" ))
  {
    // dataPort = 0;
    client << "200 Zzz...\r\n";
  }
  //
  //  RETR - Retrieve
  //
  else if( ! strcmp( command, "RETR" ))
  {
    char path[ FTP_CWD_SIZE ];
    if( strlen( parameters ) == 0 )
      client << "501 No file name\r\n";
    else if( makePath( path ))
    {
      if( ! FAT.exists( path ))
        client << "550 File " << parameters << " not found\r\n";
      else if( ! file.open( path, O_READ ))
        client << "450 Can't open " << parameters << "\r\n";
      else if( ! dataConnect())
        client << "425 No data connection\r\n";
      else
      {
        #ifdef FTP_DEBUG
          Serial << "Sending " << parameters << endl;
        #endif
        client << "150-Connected to port " << dataPort << "\r\n";
        client << "150 " << file.fileSize() << " bytes to download\r\n";
        millisBeginTrans = millis();
        bytesTransfered = 0;
        transferStatus = 1;
      }
    }
  }
  //
  //  STOR - Store
  //
  else if( ! strcmp( command, "STOR" ))
  {
    char path[ FTP_CWD_SIZE ];
    if( strlen( parameters ) == 0 )
      client << "501 No file name\r\n";
    else if( makePath( path ))
    {
      if( ! file.open( path, O_CREAT | O_WRITE ))
        client << "451 Can't open/create " << parameters << "\r\n";
      else if( ! dataConnect())
      {
        client << "425 No data connection\r\n";
        file.close();
      }
      else
      {
        #ifdef FTP_DEBUG
          Serial << "Receiving " << parameters << endl;
        #endif
        client << "150 Connected to port " << dataPort << "\r\n";
        millisBeginTrans = millis();
        bytesTransfered = 0;
        transferStatus = 2;
      }
    }
  }
  //
  //  MKD - Make Directory
  //
  else if( ! strcmp( command, "MKD" ))
  {
    char path[ FTP_CWD_SIZE ];
    if( strlen( parameters ) == 0 )
      client << "501 No directory name\r\n";
    else if( makePath( path ))
    {
      if( FAT.exists( path ))
        client << "521 \"" << parameters << "\" directory already exists\r\n";
      else
      {
        #ifdef FTP_DEBUG
          Serial << "Creating directory " << parameters << endl;
        #endif
        if( FAT.mkdir( path ))
          client << "257 \"" << parameters << "\" created\r\n";
        else
          client << "550 Can't create \"" << parameters << "\"\r\n";
      }
    }
  }
  //
  //  RMD - Remove a Directory 
  //
  else if( ! strcmp( command, "RMD" ))
  {
    char path[ FTP_CWD_SIZE ];
    if( strlen( parameters ) == 0 )
      client << "501 No directory name\r\n";
    else if( makePath( path ))
    {
      #ifdef FTP_DEBUG
        Serial << "Deleting " << path << endl;
      #endif
      if( ! FAT.exists( path ))
        client << "550 File " << parameters << " not found\r\n";
      else if( FAT.rmdir( path ))
        client << "250 \"" << parameters << "\" deleted\r\n";
      else
        client << "501 Can't delete \"" << parameters << "\"\r\n";
    }
  }
  //
  //  RNFR - Rename From 
  //
  else if( ! strcmp( command, "RNFR" ))
  {
    buf[ 0 ] = 0;
    if( strlen( parameters ) == 0 )
      client << "501 No file name\r\n";
    else if( makePath( buf ))
    {
      if( ! FAT.exists( buf ))
        client << "550 File " << parameters << " not found\r\n";
      else
      {
        #ifdef FTP_DEBUG
          Serial << "Renaming " << buf << endl;
        #endif
        client << "350 RNFR accepted - file exists, ready for destination\r\n";     
        rnfrCmd = true;
      }
    }
  }
  //
  //  RNTO - Rename To 
  //
  else if( ! strcmp( command, "RNTO" ))
  {
  Serial << buf << endl;
    char path[ FTP_CWD_SIZE ];
    char dir[ FTP_FIL_SIZE ];
    if( strlen( buf ) == 0 || ! rnfrCmd )
      client << "503 Need RNFR before RNTO\r\n";
    else if( strlen( parameters ) == 0 )
      client << "501 No file name\r\n";
    else if( makePath( path ))
    {
      if( FAT.exists( path ))
        client << "553 " << parameters << " already exists\r\n";
      else
      {
        strcpy( dir, path );
        char * psep = strrchr( dir, '/' );
        boolean fail = psep == NULL;
        if( ! fail )
        {
          if( psep == dir )
            psep ++;
          * psep = 0;
          #if FAT_SYST == 0
            fail = ! file.open( dir ) || ! file.isDir();
            file.close();
          #else
            fail = ! FAT.isDir( dir );
          #endif
          if( fail )
            client << "550 \"" << dir << "\" is not directory\r\n";
          else
          {
            #ifdef FTP_DEBUG
              Serial << "Renaming " << buf << " to " << path << endl;
            #endif
            if( FAT.rename( buf, path ))
              client << "250 File successfully renamed or moved\r\n";
            else
              fail = true;
          }
        }
        if( fail )
          client << "451 Rename/move failure\r\n";
      }
    }
    rnfrCmd = false;
  }

  ///////////////////////////////////////
  //                                   //
  //   EXTENSIONS COMMANDS (RFC 3659)  //
  //                                   //
  ///////////////////////////////////////

  //
  //  FEAT - New Features
  //
  else if( ! strcmp( command, "FEAT" ))
  {
    client << "211-Extensions suported:\r\n";
    client << " MDTM\r\n";
    client << " MLSD\r\n";
    client << " SIZE\r\n";
    client << " SITE FREE\r\n";
    client << "211 End.\r\n";
  }
  //
  //  MDTM - File Modification Time (see RFC 3659)
  //
  else if( ! strcmp( command, "MDTM" ))
  {
    char path[ FTP_CWD_SIZE ];
    char * fname = parameters;
    uint16_t year;
    uint8_t month, day, hour, minute, second, setTime;
    setTime = getDateTime( & year, & month, & day, & hour, & minute, & second );
    // fname point to file name
    fname += setTime;
    if( strlen( fname ) <= 0 )
      client << "501 No file name\r\n";
    else if( makePath( path, fname ))
    {
      if( ! FAT.exists( path ))
        client << "550 No such file " << parameters << "\r\n";
      else if( setTime ) // set file modification time
      {
        if( FAT.timeStamp( path, year, month, day, hour, minute, second ))
          client << "200 Ok\r\n";
        else
          client << "550 Unable to modify time\r\n";
      }
      else // get file modification time
      {
        uint16_t date, time;
        if( FAT.getFileModTime( path, & date, & time ))
        {
          char dtStr[ 15 ];
          client << "213 " << makeDateTimeStr( dtStr, date, time ) << "\r\n";
        }
        else
          client << "550 Unable to retrieve time\r\n";
      }
    }
  }
  //
  //  SIZE - Size of the file
  //
  else if( ! strcmp( command, "SIZE" ))
  {
    char path[ FTP_CWD_SIZE ];
    if( strlen( parameters ) == 0 )
      client << "501 No file name\r\n";
    else if( makePath( path ))
    {
      if( ! FAT.exists( path ))
        client << "550 No such file " << parameters << "\r\n";
      else if( ! file.open( path ))
        client << "450 Can't open " << parameters << "\r\n";
      else
      {
        client << "213 " << file.fileSize() << "\r\n";
        file.close();
      }
    }
  }
  //
  //  SITE - System command
  //
  else if( ! strcmp( command, "SITE" ))
  {
    if( ! strcmp( parameters, "FREE" ))
      client << "200 " << FAT.free() << " MB free of " 
             << FAT.capacity() << " MB capacity\r\n";
    else
      client << "500 Unknow SITE command " << parameters << "\r\n";
  }
  //
  //  Unrecognized commands ...
  //
  else
    client << "500 Unknow command\r\n";
  
  return true;
}

int FtpServer::dataConnect()
{
  if( ! data.connected() )
    if( dataPassiveConn )
      data = dataServer.connected();
    else
      data.connect( dataIp, dataPort );
  return data;
}

boolean FtpServer::doRetrieve()
{
  int16_t nb = file.read( buf, FTP_BUF_SIZE );
  if( nb > 0 )
  {
    data.write( buf, nb );
    bytesTransfered += nb;
    return true;
  }
  closeTransfer();
  return false;
}

boolean FtpServer::doStore()
{
  if( data.connected() )
  {
    int16_t nb = data.read((uint8_t *) buf, FTP_BUF_SIZE );
    if( nb > 0 )
    {
      // Serial << millis() << " " << nb << endl;
      file.write( buf, nb );
      bytesTransfered += nb;
    }
    return true;
  }
  closeTransfer();
  return false;
}

void FtpServer::closeTransfer()
{
  uint32_t deltaT = (int32_t) ( millis() - millisBeginTrans );
  if( deltaT > 0 && bytesTransfered > 0 )
  {
    client << "226-File successfully transferred\r\n";
    client << "226 " << deltaT << " ms, "
           << bytesTransfered / deltaT << " kbytes/s\r\n";
  }
  else
    client << "226 File successfully transferred\r\n";
  
  file.close();
  data.stop();
}

void FtpServer::abortTransfer()
{
  if( transferStatus > 0 )
  {
    file.close();
    data.stop(); 
    client << "426 Transfer aborted" << "\r\n";
    #ifdef FTP_DEBUG
      Serial << "Transfer aborted!" << endl;
    #endif
  }
  transferStatus = 0;
}

// Read a char from client connected to ftp server
//
//  update cmdLine and command buffers, iCL and parameters pointers
//
//  return:
//    -2 if buffer cmdLine is full
//    -1 if line not completed
//     0 if empty line received
//    length of cmdLine (positive) if no empty line received 

int8_t FtpServer::readChar()
{
  int8_t rc = -1;

  if( client.available())
  {
    char c = client.read();
    #ifdef FTP_DEBUG
      Serial << c;
    #endif
    if( c == '\\' )
      c = '/';
    if( c != '\r' )
      if( c != '\n' )
      {
        if( iCL < FTP_CMD_SIZE )
          cmdLine[ iCL ++ ] = c;
        else
          rc = -2; //  Line too long
      }
      else
      {
        cmdLine[ iCL ] = 0;
        command[ 0 ] = 0;
        parameters = NULL;
        // empty line?
        if( iCL == 0 )
          rc = 0;
        else
        {
          rc = iCL;
          // search for space between command and parameters
          parameters = strchr( cmdLine, ' ' );
          if( parameters != NULL )
          {
            if( parameters - cmdLine > 4 )
              rc = -2; // Syntax error
            else
            {
              strncpy( command, cmdLine, parameters - cmdLine );
              command[ parameters - cmdLine ] = 0;
              
              while( * ( ++ parameters ) == ' ' )
                ;
            }
          }
          else if( strlen( cmdLine ) > 4 )
            rc = -2; // Syntax error.
          else
            strcpy( command, cmdLine );
          iCL = 0;
        }
      }
    if( rc > 0 )
      for( uint8_t i = 0 ; i < strlen( command ); i ++ )
        command[ i ] = toupper( command[ i ] );
    if( rc == -2 )
    {
      iCL = 0;
      client << "500 Syntax error\r\n";
    }
  }
  return rc;
}

// Make complete path/name from cwdName and parameters
//
// 3 possible cases: parameters can be absolute path, relative path or only the name
//
// parameters:
//   fullName : where to store the path/name
//
// return:
//    true, if done

boolean FtpServer::makePath( char * fullName )
{
  return makePath( fullName, parameters );
}

boolean FtpServer::makePath( char * fullName, char * param )
{
  if( param == NULL )
    param = parameters;
    
  // Root or empty?
  if( strcmp( param, "/" ) == 0 || strlen( param ) == 0 )
  {
    strcpy( fullName, "/" );
    return true;
  }
  // If relative path, concatenate with current dir
  if( param[0] != '/' ) 
  {
    strcpy( fullName, cwdName );
    if( fullName[ strlen( fullName ) - 1 ] != '/' )
      strncat( fullName, "/", FTP_CWD_SIZE );
    strncat( fullName, param, FTP_CWD_SIZE );
  }
  else
    strcpy( fullName, param );
  // If ends with '/', remove it
  uint16_t strl = strlen( fullName ) - 1;
  if( fullName[ strl ] == '/' && strl > 1 )
    fullName[ strl ] = 0;
  if( strlen( fullName ) < FTP_CWD_SIZE )
    return true;

  client << "500 Command line too long\r\n";
  return false;
}

// Calculate year, month, day, hour, minute and second
//   from first parameter sent by MDTM command (YYYYMMDDHHMMSS)
//
// parameters:
//   pyear, pmonth, pday, phour, pminute and psecond: pointer of
//     variables where to store data
//
// return:
//    0 if parameter is not YYYYMMDDHHMMSS
//    length of parameter + space

uint8_t FtpServer::getDateTime( uint16_t * pyear, uint8_t * pmonth, uint8_t * pday,
                                uint8_t * phour, uint8_t * pminute, uint8_t * psecond )
{
  char dt[ 15 ];

  // Date/time are expressed as a 14 digits long string
  //   terminated by a space and followed by name of file
  if( strlen( parameters ) < 15 || parameters[ 14 ] != ' ' )
    return 0;
  for( uint8_t i = 0; i < 14; i++ )
    if( ! isdigit( parameters[ i ]))
      return 0;

  strncpy( dt, parameters, 14 );
  dt[ 14 ] = 0;
  * psecond = atoi( dt + 12 ); 
  dt[ 12 ] = 0;
  * pminute = atoi( dt + 10 );
  dt[ 10 ] = 0;
  * phour = atoi( dt + 8 );
  dt[ 8 ] = 0;
  * pday = atoi( dt + 6 );
  dt[ 6 ] = 0 ;
  * pmonth = atoi( dt + 4 );
  dt[ 4 ] = 0 ;
  * pyear = atoi( dt );
  return 15;
}

// Create string YYYYMMDDHHMMSS from date and time
//
// parameters:
//    date, time 
//    tstr: where to store the string. Must be at least 15 characters long
//
// return:
//    pointer to tstr

char * FtpServer::makeDateTimeStr( char * tstr, uint16_t date, uint16_t time )
{
  sprintf( tstr, "%04u%02u%02u%02u%02u%02u",
           (( date & 0xFE00 ) >> 9 ) + 1980, ( date & 0x01E0 ) >> 5, date & 0x001F,
           ( time & 0xF800 ) >> 11, ( time & 0x07E0 ) >> 5, ( time & 0x001F ) << 1 );            
  return tstr;
}

