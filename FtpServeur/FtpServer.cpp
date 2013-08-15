
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

#include "FtpServer.h"
#include "LongNames.h"

EthernetServer ftpServer( 21 );
    
void FtpServer::init()
{
  // Tells the ftp server to begin listening for incoming connection
  ftpServer.begin();
  
  // Default for data port
  dataPort = 20;

  // Set the root directory
  strcpy( cwdLName, "/" );
  strcpy( cwdSName, "/" );

  cmdStatus = 0;
  transferStatus = 0;
  millisTimeOut = FTP_TIME_OUT * 60 * 1000;
}

void FtpServer::service()
{
  if( cmdStatus == 0 )
  {
    if( client.connected())
    {
      Serial << "Closing client" << endl;
      client.stop();
    }
    // ftpServer.begin();
    Serial << "Ftp server waiting for connection on port 21." << endl;
    cmdStatus = 1;
  }
  else if( cmdStatus == 1 )  // Ftp server idle
  {
    client = ftpServer.connected();
    if( client == true )   // A client connected
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
      cmdStatus = 0;
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
  Serial << "Client connected!" << endl;
  client << "220---Welcome to FTP for Arduino---\r\n";
  client << "220 Version " << FTP_SERVER_VERSION << "\r\n";
  iCL = 0;
}

void FtpServer::disconnectClient()
{
  Serial << " Disconnecting client" << endl;
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
    strcpy( cwdLName, "/" );
    strcpy( cwdSName, "/" );
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
    Serial << "OK. Waiting for commands." << endl;
    client << "230 OK.\r\n";
    return true;
  }
  disconnectClient();
  return false;
}

boolean FtpServer::processCommand()
{
  //  ABOR - Abort
  if( ! strcmp( command, "ABOR" ))
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
  //  CDUP - Change to Parent Directory 
  else if( ! strcmp( command, "CDUP" ))
  {
    if( strlen( cwdLName ) > 1 )
    {
      char * pSep = strrchr( cwdLName, '/' );
      if( pSep > cwdLName )
      {
        char tmp[ FTP_CWD_SIZE ];
        char shortPath[ FTP_CWD_SIZE ];
        strcpy( tmp, cwdLName );
        tmp[ pSep - cwdLName + 1 ] = 0;
        boolean ok = l2sPath( shortPath, tmp, FTP_CWD_SIZE );
        if( ok )
          ok = sd.chdir( shortPath );
        if( ok )
        {
          strcpy( cwdLName, tmp );
          cwdLName[ strlen( tmp ) - 1 ] = 0; // elimine le dernier '/'
          strcpy( cwdSName, shortPath );
          // Serial << shortPath << endl; // for debugging
        }
      }
      else
      {
        strcpy( cwdLName, "/" );
        strcpy( cwdSName, "/" );
      }  
    }
    client << "200 Ok. Current directory is " << cwdLName << "\r\n";
  }
  //  CWD - Change Working Directory
  else if( ! strcmp( command, "CWD" ))
  {
    if( ! strcmp( parameters, "." ))  // 'CWD .' is the same as PWD command
      client << "257 \"" << cwdLName << "\" is your current directory\r\n";
    else
    {
      boolean ok = true;
      char tmp[ FTP_CWD_SIZE ];
      char shortPath[ FTP_CWD_SIZE ];
      if( ! strcmp( parameters, "/" ) ||    // 
          strlen( parameters ) == 0 )       // no parameters
      {
        strcpy( cwdLName, "/" );            // go to root
        strcpy( cwdSName, "/" );
      }
      else
      {
        if( parameters[0] != '/' ) // relative path. Concatenate with current dir
        {
          strcpy( tmp, cwdLName );
          if( tmp[ strlen( tmp ) - 1 ] != '/' )
            strcat( tmp, "/" );
          strcat( tmp, parameters );
        }
        else
          strcpy( tmp, parameters );
        if( tmp[ strlen( tmp ) - 1 ] != '/' )
          strcat( tmp, "/" );
        ok = l2sPath( shortPath, tmp, FTP_CWD_SIZE ); // compute short name of dir
        if( ok )
          ok = sd.chdir( shortPath );   // try to change to new dir
        if( ok )
        {
          strcpy( cwdLName, tmp );
          cwdLName[ strlen( tmp ) - 1 ] = 0;  // elimine le dernier '/'
          strcpy( cwdSName, shortPath );
          // Serial << shortPath << endl; // for debugging
        }
      }
      if( ok )
        client << "250 Ok. Current directory is " << cwdLName << "\r\n";
      else
        client << "550 Can't change directory to " << parameters << "\r\n";
    }
  }
  //  FEAT - New Features
  else if( ! strcmp( command, "FEAT" ))
  {
    client << "211-Extensons suported:\r\n";
    client << " MLSD\r\n";
    client << " SIZE\r\n";
    client << "211 End.\r\n";
  }
  //  MLSD - Listing for Machine Processing (see RFC 3659)
  else if( ! strcmp( command, "MLSD" ))
  {
    if( ! data.connect( dataIp, dataPort ))
      client << "425 No data connection\r\n";
    else
    {
      client << "150 Accepted data connection\r\n";
      uint16_t nm = 0;
      uint8_t fT;
      char fileName[131];
      char * fileType[4] = { "cdir", "pdir", "dir", "file" };
      // point to directory structure of first file/subdirectory
      dir_t dir = dirLfnFirst( fileName, cwdSName );
      while( strlen( fileName ) > 0 )
      {
        if(( dir.attributes & DIR_ATT_FILE_TYPE_MASK ) != DIR_ATT_DIRECTORY ) // file ?
          fT = 3;
        else
          if( ! strcmp( fileName, "." ))
            fT = 0;
          else if( ! strcmp( fileName, ".." ))
            fT = 1;
          else
            fT = 2;
        data << "Type=" << fileType[ fT ] << ";"
             << "Size=" << dir.fileSize << "; " << fileName << "\r\n";
        // point to directory structure of next file/subdirectory
        dir = dirLfnNext( fileName );
        nm ++;
      }
      client << "226-options: -a -l\r\n";
      client << "226 " << nm << " matches total\r\n";
      data.stop();
    }
  }
  //  NLST - Name List 
  //  weak implementation of LIST
  else if( ! strcmp( command, "NLST" ) || ! strcmp( command, "LIST" ) )
  {
    if( ! data.connect( dataIp, dataPort ))
      client << "425 No data connection\r\n";
    else
    {
      client << "150 Accepted data connection\r\n";
      uint16_t nm = 0;
      char fileName[131];
      dirLfnFirst( fileName, cwdSName ); // get long name of first file/directory 
      while( strlen( fileName ) > 0 )
      {
        data << fileName << "\r\n";
        dirLfnNext( fileName );   // get long name of next file/directory
        nm ++;
      }
      client << "226-options: -a\r\n";
      client << "226 " << nm << " matches total\r\n";
      data.stop();
    }
  }
  //  MODE - Transfer Mode 
  else if( ! strcmp( command, "MODE" ))
  {
    if( ! strcmp( parameters, "S" ))
      client << "200 S Ok\r\n";
    // else if( ! strcmp( parameters, "B" ))
    //  client << "200 B Ok\r\n";
    else
      client << "504 Only S(tream) is suported\r\n";
  }
  //  NOOP
  else if( ! strcmp( command, "NOOP" ))
  {
    dataPort = 0;
    client << "200 Zzz...\r\n";
  }
  //  PORT - Data Port
  else if( ! strcmp( command, "PORT" ))
  {
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
      Serial << "Data IP set to " << dataIp[0] << ":" << dataIp[1]
             << ":" << dataIp[2] << ":" << dataIp[3] << endl;
      Serial << "Data port set to " << dataPort << endl;
      client << "200 PORT command successful\r\n";
    }
  }
  //  PWD - Print Directory
  else if( ! strcmp( command, "PWD" ))
    client << "257 \"" << cwdLName << "\" is your current directory\r\n";
  //  QUIT
  else if( ! strcmp( command, "QUIT" ))
  {
    client << "221 Goodbye\r\n";
    disconnectClient();
    return false;
  }
  //  RETR - Retrieve
  else if( ! strcmp( command, "RETR" ))
  {
    char fullFileName[ 252 ];
    if( ! l2sName( fullFileName , cwdSName, parameters ))
      client << "550 No such file " << parameters << "\r\n";
    else if( ! file.open( fullFileName, O_READ ))
      client << "450 Can't open " << fullFileName << "\r\n";
    else if( ! data.connect( dataIp, dataPort ))
    {
      client << "425 No data connection\r\n";
      file.close();
    }
    else
    {
      client << "150-Connected to port " << dataPort << "\r\n";
      client << "150 " << file.fileSize() << " bytes to download\r\n";
      millisBeginTrans = millis();
      bytesTransfered = 0;
      transferStatus = 1;
    }
  }
  //  SIZE - Size of the file
  else if( ! strcmp( command, "SIZE" ))
  {
    char fullFileName[ 252 ];
    if( strlen( parameters ) == 0 )
      client << "501 No file name\r\n";
    else if( ! l2sName( fullFileName , cwdSName, parameters ))
      client << "550 No such file " << parameters << "\r\n";
    else if( ! file.open( fullFileName, O_READ ))
      client << "450 Can't open " << fullFileName << "\r\n";
    else
    {
      client << "213 " << file.fileSize() << "\r\n";
      file.close();
    }
  }
  //  STOR - Store
  else if( ! strcmp( command, "STOR" ))
  {
    char fullFileName[ 252 ];
    bool ok = false;
    if( strlen( parameters ) == 0 )
      client << "501 No file name\r\n";
    else
    {
      if( l2sName( fullFileName , cwdSName, parameters ))  // file exist. Overwrite it
        ok = file.open( fullFileName, O_TRUNC | O_RDWR );
      else
      {
        strcpy( fullFileName, cwdSName );
        strcat( fullFileName, parameters );
        ok = file.open( fullFileName, O_CREAT | O_RDWR );
      }
      if( ! ok )
        client << "451 Can't open/create " << fullFileName << "\r\n";
      else if( ! data.connect( dataIp, dataPort ))
      {
        client << "425 No data connection\r\n";
        file.close();
      }
      else
      {
        client << "150 Connected to port " << dataPort << "\r\n";
        millisBeginTrans = millis();
        bytesTransfered = 0;
        transferStatus = 2;
      }
    }
  }
  //  STRU - File Structure
  else if( ! strcmp( command, "STRU" ))
  {
    if( ! strcmp( parameters, "F" ))
      client << "200 F Ok\r\n";
    // else if( ! strcmp( parameters, "R" ))
    //  client << "200 B Ok\r\n";
    else
      client << "504 Only F(ile) is suported\r\n";
  }
  //  TYPE - Data Type
  else if( ! strcmp( command, "TYPE" ))
  {
    if( ! strcmp( parameters, "A" ))
      client << "200 TYPE is now ASII\r\n";
    else if( ! strcmp( parameters, "I" ))
      client << "200 TYPE is now 8-bit binary\r\n";
    else
      client << "504 Unknow TYPE\r\n";
  }
  //  Unrecognized commands ...
  else
    client << "500 Unknow command\r\n";
  
  return true;
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
    Serial << c;
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

