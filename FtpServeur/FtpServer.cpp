
/*
 * FTP Serveur for Arduino Due and Ethernet shield (W5100) or WIZ820io (W5200)
 * Copyright (c) 2014 by Jean-Michel Gallego
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
 *   FEAT, SIZE
 *   SITE FREE
 *
 * Tested with those clients:
 *   under Windows:
 *     FTP Rush : ok
 *     Filezilla : problem with RETR and STOR
 *   under Ubuntu:
 *     gFTP : ok
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

SdList sdl;
   
void FtpServer::init()
{
  // Tells the ftp server to begin listening for incoming connection
  ftpServer.begin();
  dataServer.begin();
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

  cwdRNFR[ 0 ] = 0;
  cmdStatus = 0;
  transferStatus = 0;
  millisTimeOut = ( uint32_t ) FTP_TIME_OUT * 60 * 1000;
}

void FtpServer::service()
{
  if( cmdStatus == 0 )
  {
    if( client.connected())
    {
      #ifdef FTP_DEBUG
        Serial << "Closing client" << endl;
      #endif
      client.stop();
    }
    #ifdef FTP_DEBUG
      Serial << "Ftp server waiting for connection on port " << FTP_CTRL_PORT << endl;
    #endif
    cmdStatus = 1;
  }
  else if( cmdStatus == 1 )  // Ftp server idle
  {
    client = ftpServer.connected();
    if( client > 0 )   // A client connected
    {
      clientConnected();      
      millisEndConnection = millis() + 10 * 1000 ; // wait client id during 10 s.
      cmdStatus = 2;
    }
  }
  else
    if( ! client.connected() )
    {
      disconnectClient();
      iniVariables();
    }
    else if( readChar() > 0 )         // got response
      if( cmdStatus == 2 )            // Ftp server waiting for user registration
        if( userIdentity() )
          cmdStatus = 3;
        else
          cmdStatus = 0;
      else if( cmdStatus == 3 )       // Ftp server waiting for user registration
        if( userPassword() )
        {
          cmdStatus = 4;
          millisEndConnection = millis() + millisTimeOut;
        }
        else
          cmdStatus = 0;
      else if( cmdStatus == 4 )       // Ftp server waiting for user command
        if( ! processCommand())
          cmdStatus = 0;
        else
          millisEndConnection = millis() + millisTimeOut;
 
  if( transferStatus == 1 )           // Retrieve data
  {
    if( ! doRetrieve())
      transferStatus = 0;
  }
  else if( transferStatus == 2 )      // Store data
  {
    if( ! doStore())
      transferStatus = 0;
  }
  else if( cmdStatus > 1 && ! ((int32_t) ( millisEndConnection - millis() ) > 0 ))
  {
    client << "530 Timeout\r\n";
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
  client.stop();
}

boolean FtpServer::userIdentity()
{
  if( strcmp( command, "USER" ))
    client << "500 Syntax error\r\n";
  else if( strcmp( parameters, FTP_USER ))
    client << "530 \r\n";
  else
  {
    client << "331 OK. Password required\r\n";
    strcpy( cwdName, "/" );
    return true;
  }
  disconnectClient();
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
  disconnectClient();
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
    char * pSep;
    char tmp[ FTP_CWD_SIZE ];
    boolean ok = false;
    
    if( strlen( cwdName ) > 1 )
    {
      // if cwdName ends with '/', remove it
      if( cwdName[ strlen( cwdName ) - 1 ] == '/' )
        cwdName[ strlen( cwdName ) - 1 ] = 0;
      // search last '/'
      pSep = strrchr( cwdName, '/' );
      ok = pSep > cwdName;
      // if found, ends the string after its position
      if( ok )
      {
        * ( pSep + 1 ) = 0;
        ok = sdl.chdir( cwdName );
      }
    }
    // if an error appends, move to root
    if( ! ok )
    {
      strcpy( cwdName, "/" );
      sdl.chdir( cwdName );
    }
    // Serial << cwdName << endl; // for debugging
    client << "200 Ok. Current directory is " << cwdName << "\r\n";
  }
  //
  //  CWD - Change Working Directory
  //
  else if( ! strcmp( command, "CWD" ))
  {
    if( strcmp( parameters, "." ) == 0 )  // 'CWD .' is the same as PWD command
      client << "257 \"" << cwdName << "\" is your current directory\r\n";
    else
    {
      boolean ok = true;
      char tmp[ FTP_CWD_SIZE ];
      if( strcmp( parameters, "/" ) == 0 || strlen( parameters ) == 0 )
        strcpy( cwdName, "/" );            // go to root
      else
      {
        if( parameters[0] != '/' ) // relative path. Concatenate with current dir
        {
          strcpy( tmp, cwdName );
          if( tmp[ strlen( tmp ) - 1 ] != '/' )
            strcat( tmp, "/" );
          strcat( tmp, parameters );
        }
        else
          strcpy( tmp, parameters );
        if( tmp[ strlen( tmp ) - 1 ] != '/' )
          strcat( tmp, "/" );
        ok = sdl.chdir( tmp );   // try to change to new dir
        if( ok )
        {
          strcpy( cwdName, tmp );
          // Serial << cwdName << endl; // for debugging
        }
      }
      if( ok )
        client << "250 Ok. Current directory is " << cwdName << "\r\n";
      else
        client << "550 Can't change directory to " << parameters << "\r\n";
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
    client << "221 Goodbye\r\n";
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
    if( transferStatus > 0 )
    {
      file.close();
      data.stop(); 
      client << "426 Transfer aborted" << "\r\n";
      transferStatus = 0;
    }
    client << "226 Data connection closed" << "\r\n";
  }
  //
  //  DELE - Delete a File 
  //
  else if( ! strcmp( command, "DELE" ))
  {
    if( strlen( parameters ) == 0 )
      client << "501 No file name\r\n";
    else
    {
      char path[ FTP_CWD_SIZE ];
      char name[ FTP_FIL_SIZE ];
      makePathName( name, path, FTP_CWD_SIZE );
      // Serial << "Deleting [" << name << "] in [" << path << "]" << endl;
      if( ! sdl.chdir( path ) || ! sdl.exists( name ))
        client << "550 File " << parameters << " not found\r\n";
      else
      {
        if( sdl.remove( name ))
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
      char fileName[ FTP_FIL_SIZE ];
      bool isFile;
      uint32_t fileSize;
      uint16_t nm = 0;
      //sdl.vwd()-> rewind();
      sdl.chdir( cwdName );
      while( sdl.nextFile( fileName, & isFile, & fileSize ))
      {
        if( isFile )
          data << "+r,s" << fileSize;
        else
          data << "+/";
        data << ",\t" << fileName << "\r\n";
        nm ++;
      }
      client << "226 " << nm << " matches total\r\n";
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
      char fileName[ FTP_FIL_SIZE ];
      bool isFile;
      uint32_t fileSize;
      uint16_t nm = 0;
      sdl.chdir( cwdName );
      while( sdl.nextFile( fileName, & isFile, & fileSize ))
      {
        data << "Type=" << ( isFile ? "file" : "dir" ) << ";"
             << "Size=" << fileSize << "; " << fileName << "\r\n";
        nm ++;
      }
      client << "226-options: -a -l\r\n";
      client<< "226 " << nm << " matches total\r\n";
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
      char fileName[ FTP_FIL_SIZE ];
      bool isFile;
      uint32_t fileSize;
      uint16_t nm = 0;
      sdl.chdir( cwdName );
      while( sdl.nextFile( fileName, & isFile, & fileSize ))
      {
        data << fileName << "\r\n";
        nm ++;
      }
      client << "226 " << nm << " matches total\r\n";
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
    if( strlen( parameters ) == 0 )
      client << "501 No file name\r\n";
    else
    {
      char path[ FTP_CWD_SIZE ];
      char name[ FTP_FIL_SIZE ];
      makePathName( name, path, FTP_CWD_SIZE );
      if( ! sdl.chdir( path ) || ! sdl.exists( name ))
        client << "550 File " << parameters << " not found\r\n";
      else
      {
        if( ! sdl.openFile( & file, name, O_READ ))
          client << "450 Can't open " << parameters << "\r\n";
        else
        {
          if( ! dataConnect())
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
    }
  }
  //
  //  STOR - Store
  //
  else if( ! strcmp( command, "STOR" ))
  {
    if( strlen( parameters ) == 0 )
      client << "501 No file name\r\n";
    else
    {
      char path[ FTP_CWD_SIZE ];
      char name[ FTP_FIL_SIZE ];
      makePathName( name, path, FTP_CWD_SIZE );
      if( ! sdl.chdir( path ) || ! sdl.openFile( & file, name, O_CREAT | O_TRUNC | O_RDWR ))
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
    if( strlen( parameters ) == 0 )
      client << "501 No directory name\r\n";
    else
    {
      char path[ FTP_CWD_SIZE ];
      char dir[ FTP_FIL_SIZE ];
      makePathName( dir, path, FTP_CWD_SIZE );
      #ifdef FTP_DEBUG
        Serial << "Creating directory " << dir << " in " << path << endl;
      #endif
      boolean ok = sdl.chdir( path );
      if( ok )
        if(  sdl.exists( dir ))
          client << "521 \"" << parameters << "\" directory already exists\r\n";
        else
        {
          ok = sdl.mkdir( dir );
          if( ok )
            client << "257 \"" << parameters << "\" created\r\n";
        }
      if( ! ok )
        client << "550 Can't create \"" << parameters << "\"\r\n";
    }
  }
  //
  //  RMD - Remove a Directory 
  //
  else if( ! strcmp( command, "RMD" ))
  {
    if( strlen( parameters ) == 0 )
      client << "501 No directory name\r\n";
    else
    {
      char path[ FTP_CWD_SIZE ];
      char dir[ FTP_FIL_SIZE ];
      makePathName( dir, path, FTP_CWD_SIZE );
      #ifdef FTP_DEBUG
        Serial << "Deleting " << dir << " in " << path << endl;
      #endif
      if( ! sdl.chdir( path ) || ! sdl.exists( dir ))
        client << "550 File " << parameters << " not found\r\n";
      else if( sdl.rmdir( dir ))
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
    cwdRNFR[ 0 ] = 0;
    if( strlen( parameters ) == 0 )
      client << "501 No file name\r\n";
    else
    {
      char dir[ FTP_FIL_SIZE ];
      makePathName( dir, cwdRNFR, FTP_CWD_SIZE );
      #ifdef FTP_DEBUG
        Serial << "Renaming " << dir << " in " << cwdRNFR << endl;
      #endif
      if( ! sdl.chdir( cwdRNFR ) || ! sdl.exists( dir ))
        client << "550 File " << parameters << " not found\r\n";
      else if( strlen( cwdRNFR ) + strlen( dir ) + 1 > FTP_CWD_SIZE )
        client << "500 Command line too long\r\n";
      else
      {
        if( cwdRNFR[ strlen( cwdRNFR ) - 1 ] != '/' )
          strcat( cwdRNFR, "/" );
        strcat( cwdRNFR, dir );
        client << "350 RNFR accepted - file exists, ready for destination\r\n";      
      }
    }
  }
  //
  //  RNTO - Rename To 
  //
  else if( ! strcmp( command, "RNTO" ))
  {
    if( strlen( cwdRNFR ) == 0 )
      client << "503 Need RNFR before RNTO\r\n";
    else if( strlen( parameters ) == 0 )
      client << "501 No file name\r\n";
    else
    {
      char path[ FTP_CWD_SIZE ];
      char dir[ FTP_FIL_SIZE ];
      makePathName( dir, path, FTP_CWD_SIZE );
      if( strlen( path ) + strlen( dir ) + 1 > FTP_CWD_SIZE )
        client << "500 Command line too long\r\n";
      else if( ! sdl.chdir( path ))
        client << "550 \"" << path << "\" is not directory\r\n";
      else
      {
        if( path[ strlen( path ) - 1 ] != '/' )
          strcat( path, "/" );
        strcat( path, dir );
        if( sdl.exists( path ))
          client << "553 " << parameters << " already exists\r\n";
        else
        {
          #ifdef FTP_DEBUG
            Serial << "Renaming " << cwdRNFR << " to " << path << endl;
          #endif
          if( sdl.rename( cwdRNFR, path ))
            client << "250 File successfully renamed or moved\r\n";
          else
            client << "451 Rename/move failure\r\n";
        }
      }
    }
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
    client << " MLSD\r\n";
    client << " SIZE\r\n";
    client << " SITE FREE\r\n";
    // client << " SITE NAME LONG\r\n";
    // client << " SITE NAME 8.3\r\n";
    client << "211 End.\r\n";
  }
  //
  //  SIZE - Size of the file
  //
  else if( ! strcmp( command, "SIZE" ))
  {
    if( strlen( parameters ) == 0 )
      client << "501 No file name\r\n";
    else
    /*
    // For testing l2sName()
    {
      char path[ FTP_CWD_SIZE ];
      char name[ FTP_FIL_SIZE ];
      char shortPathName[ FTP_CWD_SIZE ];
      makePathName( name, path, FTP_CWD_SIZE );
      if( path[ strlen( path ) - 1 ] != '/' )
        strcat( path, "/" );
      if( sdl.chdir( path ) && sdl.exists( name ) &&
          sdl.fullShortName( shortPathName, name, FTP_CWD_SIZE ) &&
          file.open( shortPathName, O_READ ) &&  file.isFile())
      {
        client << "213 " << file.fileSize() << "\r\n";
        file.close();
      }
      else
      {
        file.close();
        client << "550 No such file " << parameters << "\r\n";
      }
    }
    */
    // /*
    // The correct way
    {
      char path[ FTP_CWD_SIZE ];
      char name[ FTP_FIL_SIZE ];
      makePathName( name, path, FTP_CWD_SIZE );
      if( sdl.chdir( path ) && sdl.openFile( & file, name, O_READ ))
      {
        client << "213 " << file.fileSize() << "\r\n";
        file.close();
      }
      else
        client << "550 No such file " << parameters << "\r\n";
    }
    // */
  }
  //
  //  SITE - System command
  //
  else if( ! strcmp( command, "SITE" ))
  {
    if( ! strcmp( parameters, "FREE" ))
    {
      client << "200 " << sdl.free() << " MB free of " 
             << sdl.capacity() << " MB capacity\r\n";
    }
    /*
    else if( ! strcmp( parameters, "NAME LONG" ))
    {
      sdl.setNameLong( true );
      strcpy( cwdName, "/" );
      client << "200 Ok. Use long names\r\n";
      cmdStatus = 0;
    }
    else if( ! strcmp( parameters, "NAME 8.3" ))
    {
      sdl.setNameLong( false );
      strcpy( cwdName, "/" );
      client << "200 Ok. Use 8.3 names\r\n";
      cmdStatus = 0;
    }
    */
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
  if( dataPassiveConn )
  {
    if( ! data )
      data = dataServer.connected();
    return data;
  }
  else
    return data.connect( dataIp, dataPort );
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
    int16_t nb = data.read( buf, FTP_BUF_SIZE );
    if( nb > 0 )
    {
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

// Read a char from client connected to ftp server
//
//  update cmdLine and command buffers, iCL and parameters pointers
//
//  return:
//    -2 if buffer cmdLine is full
//    -1 if line not completed
//     0 if empty line received
//    length of cmdLine (positive) if no empy line received 

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
          char * pSpace = strchr( cmdLine, ' ' );
          if( pSpace != NULL )
          {
            if( pSpace - cmdLine > 4 )
              rc = -2; // Syntax error
            else
            {
              strncpy( command, cmdLine, pSpace - cmdLine );
              command[ pSpace - cmdLine ] = 0;
              parameters = pSpace + 1;
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

// Make path and name from cwdName and parameters
//
// 3 possible cases: parameters can be absolute path, relative path or only the name
//
// parameters:
//   name : where to store the name
//   path : where to store the path
//   maxpl : size of path' string
//
// return:
//    true, if convertion is done

boolean FtpServer::makePathName( char * name, char * path, size_t maxpl )
{
  // If parameter has no '/', it is the name
  if( strchr( parameters, '/' ) == NULL )
  {
    if( strlen( cwdName ) > maxpl )
      return false;
    strcpy( path, cwdName );
    strcpy( name, parameters );
  }
  else
  {
    // If parameter indicate a relative path, concatenate with current dir
    if( parameters[0] != '/' )
    {
      if( strlen( cwdName ) + strlen( parameters ) + 1 > maxpl )
        return false;
      strcpy( path, cwdName );
      if( path[ strlen( path ) - 1 ] != '/' )
        strcat( path, "/" );
      strcat( path, parameters );
    }
    else
    {
      if( strlen( parameters ) > maxpl )
        return false;
      strcpy( path, parameters );
    }
    // Extract name
    char * pName = strrchr( path, '/' );
    if( strlen( pName ) > FTP_FIL_SIZE )
      return false;
    strcpy( name, pName + 1 );
    // Remove name from path
    * ( pName + 1 ) = 0;
  }
  return true;
}

