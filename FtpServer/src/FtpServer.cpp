/*
 * FTP Serveur for Arduino Due or Mega 2580 
 * and Ethernet shield W5100, W5200 or W5500
 * Copyright (c) 2014-2018 by Jean-Michel Gallego
 *
 * Please read file ReadMe.txt for instructions 
 * 
 * Use Streaming.h from Mial Hart
 *
 * Use SdFat library from William Greiman (version 1.0.7)
 *   (see https://github.com/greiman/SdFat )
 *
 * Use Ethernet library (version 2.0.0)
 * 
 * Commands implemented: 
 *   USER, PASS, AUTH (AUTH only return 'not implemented' code)
 *   CDUP, CWD, PWD, QUIT, NOOP
 *   MODE, PASV, PORT, STRU, TYPE
 *   ABOR, DELE, LIST, NLST, MLST, MLSD
 *   APPE, RETR, STOR
 *   MKD,  RMD
 *   RNTO, RNFR
 *   MDTM, MFMT
 *   FEAT, SIZE
 *   SITE FREE
 *
 * Tested with those clients:
 *   under Windows:
 *     FTP Rush
 *     Filezilla
 *     WinSCP
 *     NcFTP, ncftpget, ncftpput
 *     Firefox
 *     command line ftp.exe
 *   under Ubuntu:
 *     gFTP
 *     Filezilla
 *     NcFTP, ncftpget, ncftpput
 *     lftp
 *     ftp
 *     Firefox
 *   under Android:
 *     AndFTP
 *     FTP Express
 *     Firefox
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

#define CommandIs( a ) ( ! strcmp_P( command, PSTR( a )))
#define ParameterIs( a ) ( ! strcmp_P( parameter, PSTR( a )))

ExtSdFat sd;

EthernetServer ftpServer( FTP_CTRL_PORT );
EthernetServer dataServer( FTP_DATA_PORT_PASV );

void FtpServer::init()
{
  // Tells the ftp server to begin listening for incoming connection
  ftpServer.begin();
  dataServer.begin();
  millisDelay = 0;
  cmdStage = FTP_Stop;
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
  transferStage = FTP_Close;
}

void FtpServer::service()
{
  #ifdef FTP_DEBUG
    // int8_t   cmdStage0 = cmdStage;
  #endif
  
  if((int32_t) ( millisDelay - millis() ) > 0 )
    return;

  if( cmdStage == FTP_Stop )
  {
    if( client.connected())
      disconnectClient();
    cmdStage = FTP_Init;
  }
  else if( cmdStage == FTP_Init )   // Ftp server waiting for connection
  {
    abortTransfer();
    iniVariables();
    #ifdef FTP_DEBUG
      Serial << F(" Ftp server waiting for connection on port ") << FTP_CTRL_PORT << eol;
    #endif
    cmdStage = FTP_Client;
  }
  else if( cmdStage == FTP_Client ) // Ftp server idle
  {
    client = ftpServer.accept();
    if( client )                        // A client connected
    {
      clientConnected();
      millisEndConnection = millis() + 1000L * FTP_AUTH_TIME_OUT; // wait client id for 10 s.
      cmdStage = FTP_User;
    }
  }
  else if( readChar() > 0 )             // got response
  {
    processCommand();
    if( cmdStage == FTP_Stop )
      millisEndConnection = millis() + 1000L * FTP_AUTH_TIME_OUT;  // wait authentication for 10 s.
    else if( cmdStage < FTP_Cmd )
      millisDelay = millis() + 200;     // delay of 100 ms
    else
      millisEndConnection = millis() + 1000L * FTP_TIME_OUT;
  }
  else if( ! client.connected() )
    cmdStage = FTP_Init;

  if( transferStage == FTP_Retrieve )   // Retrieve data
  {
    if( ! doRetrieve())
      transferStage = FTP_Close;
  }
  else if( transferStage == FTP_Store ) // Store data
  {
    if( ! doStore())
      transferStage = FTP_Close;
  }
  else if( cmdStage > FTP_Client &&
           ! ((int32_t) ( millisEndConnection - millis() ) > 0 ))
  {
    client << F("530 Timeout") << eol;
    millisDelay = millis() + 200;       // delay of 200 ms
    cmdStage = FTP_Stop;
  }

  #ifdef FTP_DEBUG
    // if( cmdStage != cmdStage0 )
    //   Serial << F("  Status: ") << cmdStage << eol;
  #endif
}

void FtpServer::clientConnected()
{
  #ifdef FTP_DEBUG
    Serial << F(" Client connected!") << eol;
  #endif
  client << F("220--- Welcome to FTP for Arduino ---") << eol;
  client << F("220---   By Jean-Michel Gallego   ---") << eol;
  client << F("220 --   Version ") << FTP_SERVER_VERSION << F("   --") << eol;
  iCL = 0;
}

void FtpServer::disconnectClient()
{
  #ifdef FTP_DEBUG
    Serial << F(" Disconnecting client") << eol;
  #endif
  abortTransfer();
  client << F("221 Goodbye") << eol;
  client.stop();
}

boolean FtpServer::processCommand()
{
  ///////////////////////////////////////
  //                                   //
  //      AUTHENTICATION COMMANDS      //
  //                                   //
  ///////////////////////////////////////

  //
  //  USER - User Identity 
  //
  if( CommandIs( "USER" ))
  {
    if( ParameterIs( FTP_USER ))
    {
      client << F("331 Ok. Password required") << eol;
      strcpy( cwdName, "/" );
      cmdStage = FTP_Pass;
    }
    else
    {
      client << F("530 ") << eol;
      cmdStage = FTP_Stop;
    }
  }
  //
  //  PASS - Password
  //
  else if( CommandIs( "PASS" ))
  {
    if( cmdStage != FTP_Pass )
    {
      client << F("503 ") << eol;
      cmdStage = FTP_Stop;
    }
    if( ParameterIs( FTP_PASS ))
    {
      #ifdef FTP_DEBUG
        Serial << F(" Authentication Ok. Waiting for commands.") << eol;
      #endif
      client << F("230 Ok") << eol;
      cmdStage = FTP_Cmd;
    }
    else
    {
      client << F("530 ") << eol;
      cmdStage = FTP_Stop;
    }
  }
  //
  //  FEAT - New Features
  //
  else if( CommandIs( "FEAT" ))
  {
    client << F("211-Extensions suported:") << eol;
    client << F(" MLST type*;modify*;size*;") << eol;
    client << F(" MLSD") << eol;
    client << F(" MDTM") << eol;
    client << F(" MFMT") << eol;
    client << F(" SIZE") << eol;
    client << F(" SITE FREE") << eol;
    client << F("211 End.") << eol;
  }
  //
  //  AUTH - Not implemented
  //
  else if( CommandIs( "AUTH" ))
    client << F("502 ") << eol;
  //
  //  Unrecognized commands at stage of authentication
  //
  else if( cmdStage < FTP_Cmd )
  {
    client << F("530 ") << eol;
    cmdStage = FTP_Stop;
  }

  ///////////////////////////////////////
  //                                   //
  //      ACCESS CONTROL COMMANDS      //
  //                                   //
  ///////////////////////////////////////

  //
  //  PWD - Print Directory
  //
  else if( CommandIs( "PWD" ) ||
           ( CommandIs( "CWD" ) && ParameterIs( "." )))
    client << F("257 \"") << cwdName << F("\"") << F(" is your current directory") << eol;
  //
  //  CDUP - Change to Parent Directory 
  //
  else if( CommandIs( "CDUP" ) ||
           ( CommandIs( "CWD" ) && ParameterIs( ".." )))
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
        ok = sd.exists( cwdName );
      }
    }
    // if an error appends, move to root
    if( ! ok )
      strcpy( cwdName, "/" );
    client << F("250 Ok. Current directory is ") << cwdName << eol;
  }
  //
  //  CWD - Change Working Directory
  //
  else if( CommandIs( "CWD" ))
  {
    char path[ FTP_CWD_SIZE ];
    if( haveParameter() && makeExistsPath( path ))
    {
      strcpy( cwdName, path );
      client << F("250 Directory changed to ") << cwdName << eol;
    }
  }
  //
  //  QUIT
  //
  else if( CommandIs( "QUIT" ))
  {
    client << F("221 Goodbye") << eol;
    disconnectClient();
    cmdStage = FTP_Stop;
  }

  ///////////////////////////////////////
  //                                   //
  //    TRANSFER PARAMETER COMMANDS    //
  //                                   //
  ///////////////////////////////////////

  //
  //  MODE - Transfer Mode 
  //
  else if( CommandIs( "MODE" ))
  {
    if( ParameterIs( "S" ))
      client << F("200 S Ok") << eol;
    else
      client << F("504 Only S(tream) is suported") << eol;
  }
  //
  //  PASV - Passive Connection management
  //
  else if( CommandIs( "PASV" ))
  {
    data.stop();
    dataServer.begin();
    dataIp = Ethernet.localIP();
    dataPort = FTP_DATA_PORT_PASV;
    #ifdef FTP_DEBUG
      Serial << F(" Connection management set to passive") << eol;
      Serial << F(" Data port set to ") << dataPort << eol;
    #endif
    client << F("227 Entering Passive Mode") << F(" (")
           << dataIp[0] << F(",") << dataIp[1] << F(",") 
           << dataIp[2] << F(",") << dataIp[3] << F(",") 
           << ( dataPort >> 8 ) << F(",") << ( dataPort & 255 ) << F(")") << eol;
    dataPassiveConn = true;
  }
  //
  //  PORT - Data Port
  //
  else if( CommandIs( "PORT" ))
  {
    data.stop();
    // get IP of data client
    dataIp[ 0 ] = atoi( parameter );
    char * p = strchr( parameter, ',' );
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
      client << F("501 Can't interpret parameters") << eol;
    else
    {
      #ifdef FTP_DEBUG
        Serial << F(" Data IP set to ") << dataIp[0] << F(":") << dataIp[1]
               << F(":") << dataIp[2] << F(":") << dataIp[3] << eol;
        Serial << F(" Data port set to ") << dataPort << eol;
      #endif
      client << F("200 PORT command successful") << eol;
      dataPassiveConn = false;
    }
  }
  //
  //  STRU - File Structure
  //
  else if( CommandIs( "STRU" ))
  {
    if( ParameterIs( "F" ))
      client << F("200 F Ok") << eol;
    // else if( ParameterIs( "R" ))
    //  client << F("200 B Ok") << eol;
    else
      client << F("504 Only F(ile) is suported") << eol;
  }
  //
  //  TYPE - Data Type
  //
  else if( CommandIs( "TYPE" ))
  {
    if( ParameterIs( "A" ))
      client << F("200 TYPE is now ASCII") << eol;
    else if( ParameterIs( "I" ))
      client << F("200 TYPE is now 8-bit binary") << eol;
    else
      client << F("504 Unknow TYPE") << eol;
  }

  ///////////////////////////////////////
  //                                   //
  //        FTP SERVICE COMMANDS       //
  //                                   //
  ///////////////////////////////////////

  //
  //  ABOR - Abort
  //
  else if( CommandIs( "ABOR" ))
  {
    abortTransfer();
    client << F("226 Data connection closed") << eol;
  }
  //
  //  DELE - Delete a File 
  //
  else if( CommandIs( "DELE" ))
  {
    char path[ FTP_CWD_SIZE ];
    if( haveParameter() && makeExistsPath( path ))
      if( sd.remove( path ))
        client << F("250 Deleted ") << parameter << eol;
      else
        client << F("450 Can't delete ") << parameter << eol;
  }
  //
  //  LIST - List && NLST - Name List
  //
  else if( CommandIs( "LIST" ) || CommandIs( "NLST" ))
  {
    boolean list = CommandIs( "LIST" );
    if( dataConnect())
    {
      uint16_t nm = 0;
      ExtDir dir;
      if( openDir( & dir ))
      {
        while( dir.nextFile())
        {
          if( list )
            if( dir.isDir() )
              data << F("+/,\t");
            else
              data << F("+r,s") << dir.fileSize() << F(",\t");
          data << dir.fileName() << eol;
          nm ++;
        }
        client << F("226 ") << nm << F(" matches total") << eol;
      }
      data.stop();
    }
  }
  //
  //  MLST - Listing for Machine Processing (see RFC 3659)
  //
  else if( CommandIs( "MLST" ))
  {
    char path[ FTP_CWD_SIZE ];
    if( haveParameter() && makeExistsPath( path ))
    {
      ExtDir dir;
      char dtStr[ 15 ];
      uint16_t dat, tim;
      if( ! dir.open( path ))
        client << F("450 Can't open ") << parameter << eol;
      else if( ! sd.getFileModTime( path, & dat, & tim ))
        client << F("550 Unable to retrieve time for ") << parameter << eol;
      else
      {
        client << F("250-Begin") << eol;
        client << F(" Type=") << ( dir.isFile() ? F("file") : F("dir"))
               << F(";Modify=") << makeDateTimeStr( dtStr, dat, tim ) 
               << F(";Size=") << dir.fileSize()
               << F("; ") << path << eol;
        client << F("250 End.") << eol;
        file.close();
      }
    }
  }
  //
  //  MLSD - Listing for Machine Processing (see RFC 3659)
  //
  else if( CommandIs( "MLSD" ))
  {
    if( dataConnect())
    {
      uint16_t nm = 0;
      ExtDir dir;
      if( openDir( & dir ))
      {
        char dtStr[ 15 ];
        while( dir.nextFile())
        {
          data << F("Type=") << ( dir.isDir() ? F("dir") : F("file"))
               << F(";Modify=") << makeDateTimeStr( dtStr, dir.fileModDate(), dir.fileModTime()) 
               << F(";Size=") << dir.fileSize() 
               << F("; ")<< dir.fileName() << eol;
          nm ++;
        }
        client << F("226-options: -a -l") << eol;
        client << F("226 ") << nm << F(" matches total") << eol;
      }
      data.stop();
    }
  }
  //
  //  NOOP
  //
  else if( CommandIs( "NOOP" ))
    client << F("200 Zzz...") << eol;
  //
  //  RETR - Retrieve
  //
  else if( CommandIs( "RETR" ))
  {
    char path[ FTP_CWD_SIZE ];
    if( haveParameter() && makeExistsPath( path ))
      if( ! file.open( path, O_READ ))
        client << F("450 Can't open ") << parameter << eol;
      else if( dataConnect( false ))
      {
        #ifdef FTP_DEBUG
          Serial << F(" Sending ") << parameter << eol;
        #endif
        client << F("150-Connected to port ") << dataPort << eol;
        client << F("150 ") << file.fileSize() << F(" bytes to download") << eol;
        millisBeginTrans = millis();
        bytesTransfered = 0;
        transferStage = FTP_Retrieve;
      }
  }
  //
  //  STOR - Store
  //
  else if( CommandIs( "STOR" ) || CommandIs( "APPE" ))
  {
    char path[ FTP_CWD_SIZE ];
    if( haveParameter() && makePath( path ))
    {
      if( ! file.open( path,
            O_CREAT | O_WRITE | ( CommandIs( "APPE" ) ? O_APPEND : 0 )))
        client << F("451 Can't open/create ") << parameter << eol;
      else if( ! dataConnect())
        file.close();
      else
      {
        #ifdef FTP_DEBUG
          Serial << F(" Receiving ") << parameter << eol;
        #endif
        millisBeginTrans = millis();
        bytesTransfered = 0;
        transferStage = FTP_Store;
      }
    }
  }
  //
  //  MKD - Make Directory
  //
  else if( CommandIs( "MKD" ))
  {
    char path[ FTP_CWD_SIZE ];
    if( haveParameter() && makePath( path ))
    {
      if( sd.exists( path ))
        client << F("521 \"") << parameter << F("\" directory already exists") << eol;
      else
      {
        #ifdef FTP_DEBUG
          Serial << F(" Creating directory ") << parameter << eol;
        #endif
        if( sd.mkdir( path ))
          client << F("257 \"") << parameter << F("\"") << F(" created") << eol;
        else
          client << F("550 Can't create \"") << parameter << F("\"") << eol;
      }
    }
  }
  //
  //  RMD - Remove a Directory 
  //
  else if( CommandIs( "RMD" ))
  {
    char path[ FTP_CWD_SIZE ];
    if( haveParameter() && makeExistsPath( path ))
      if( sd.rmdir( path ))
      {
        #ifdef FTP_DEBUG
          Serial << F(" Deleting ") << path << eol;
        #endif
        client << F("250 \"") << parameter << F("\" deleted") << eol;
      }
      else
        client << F("550 Can't remove \"") << parameter << F("\". Directory not empty?") << eol;
  }
  //
  //  RNFR - Rename From 
  //
  else if( CommandIs( "RNFR" ))
  {
    buf[ 0 ] = 0;
    if( haveParameter() && makeExistsPath( buf ))
    {
      #ifdef FTP_DEBUG
        Serial << F(" Ready for renaming ") << buf << eol;
      #endif
      client << F("350 RNFR accepted - file exists, ready for destination") << eol;
      rnfrCmd = true;
    }
  }
  //
  //  RNTO - Rename To 
  //
  else if( CommandIs( "RNTO" ))
  {
    char path[ FTP_CWD_SIZE ];
    char dir[ FTP_FIL_SIZE ];
    if( strlen( buf ) == 0 || ! rnfrCmd )
      client << F("503 Need RNFR before RNTO") << eol;
    else if( haveParameter() && makePath( path ))
    {
      if( sd.exists( path ))
        client << F("553 ") << parameter << F(" already exists") << eol;
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
          fail = ! file.open( dir ) || ! file.isDir();
          file.close();
          if( fail )
            client << F("550 \"") << dir << F("\" is not directory") << eol;
          else
          {
            #ifdef FTP_DEBUG
              Serial << F(" Renaming ") << buf << F(" to ") << path << eol;
            #endif
            if( sd.rename( buf, path ))
              client << F("250 File successfully renamed or moved") << eol;
            else
              fail = true;
          }
        }
        if( fail )
          client << F("451 Rename/move failure") << eol;
      }
    }
    rnfrCmd = false;
  }
  /*
  //
  //  SYST - System
  //
  else if( CommandIs( "SYST" ))
    client << F("215 MSDOS") << eol;
  */
  
  ///////////////////////////////////////
  //                                   //
  //   EXTENSIONS COMMANDS (RFC 3659)  //
  //                                   //
  ///////////////////////////////////////

  //
  //  MDTM && MFMT - File Modification Time (see RFC 3659)
  //
  else if( CommandIs( "MDTM" ) || CommandIs( "MFMT" ))
  {
    char path[ FTP_CWD_SIZE ];
    char * fname = parameter;
    uint16_t year;
    uint8_t month, day, hour, minute, second, setTime;
    char dt[ 15 ];
    boolean mdtm = CommandIs( "MDTM" );

    setTime = getDateTime( dt, & year, & month, & day, & hour, & minute, & second );
    // fname point to file name
    fname += setTime;
    if( strlen( fname ) <= 0 )
      client << "501 No file name" << eol;
    else if( makeExistsPath( path, fname ))
      if( setTime ) // set file modification time
      {
        if( sd.timeStamp( path, year, month, day, hour, minute, second ))
          client << "213 " << dt << eol;
        else
          client << "550 Unable to modify time" << eol;
      }
      else if( mdtm ) // get file modification time
      {
        uint16_t dat, tim;
        char dtStr[ 15 ];
        if( sd.getFileModTime( path, & dat, & tim ))
          client << "213 " << makeDateTimeStr( dtStr, dat, tim ) << eol;
        else
          client << "550 Unable to retrieve time" << eol;
      }
  }
  //
  //  SIZE - Size of the file
  //
  else if( CommandIs( "SIZE" ))
  {
    char path[ FTP_CWD_SIZE ];
    if( haveParameter() && makeExistsPath( path ))
      if( ! file.open( path ))
        client << F("450 Can't open ") << parameter << eol;
      else
      {
        client << F("213 ") << file.fileSize() << eol;
        file.close();
      }
  }
  //
  //  SITE - System command
  //
  else if( CommandIs( "SITE" ))
  {
    if( ParameterIs( "FREE" ))
      client << F("200 ") << sd.free() << F(" MB free of ") 
             << sd.capacity() << F(" MB capacity") << eol;
    else
      client << F("500 Unknow SITE command ") << parameter << eol;
  }
  //
  //  Unrecognized commands ...
  //
  else
    client << F("500 Unknow command") << eol;
  
  return true;
}

int FtpServer::dataConnect( boolean out150 )
{
  boolean dataConnected;
  //if( data )
  //  data.stop();
  if( dataPassiveConn )
  {
    uint16_t count = 1000; // wait up to a second
    while( ! data && count -- > 0 )
    {
      data = dataServer.accept();
      delay( 1 );
    }
    dataConnected = data;
  }
  else
    dataConnected = data.connect( dataIp, dataPort );

  if( ! dataConnected )
    client << F("425 No data connection") << eol;
  else if( out150 )
    client << F("150 Accepted data connection to port ") << dataPort << eol;
  
  return dataConnected;
}

boolean FtpServer::openDir( ExtDir * pdir, char * sdir )
{
  if( sdir == NULL )
    sdir = cwdName;
  boolean openD = pdir->openDir( sdir );
  if( ! openD )
    client << F("550 Can't open directory ") << sdir << eol;
  return openD;
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
      // Serial << millis() << " " << nb << eol;
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
    #ifdef FTP_DEBUG
      Serial << F(" Transfer completed in ") << deltaT << F(" ms, ")
             << bytesTransfered / deltaT << F(" kbytes/s") << eol;
    #endif
    client << F("226-File successfully transferred") << eol;
    client << F("226 ") << deltaT << F(" ms, ")
           << bytesTransfered / deltaT << F(" kbytes/s") << eol;
  }
  else
    client << F("226 File successfully transferred") << eol;
  
  file.close();
  data.stop();
}

void FtpServer::abortTransfer()
{
  if( transferStage != FTP_Close )
  {
    file.close();
    data.stop(); 
    client << F("426 Transfer aborted") << eol;
    #ifdef FTP_DEBUG
      Serial << F(" Transfer aborted!") << eol;
    #endif
    transferStage = FTP_Close;
  }
}

// Read a char from client connected to ftp server
//
//  update cmdLine and command buffers, iCL and parameter pointers
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
        parameter = NULL;
        // empty line?
        if( iCL == 0 )
          rc = 0;
        else
        {
          rc = iCL;
          // search for space between command and parameter
          parameter = strchr( cmdLine, ' ' );
          if( parameter != NULL )
          {
            if( parameter - cmdLine > 4 )
              rc = -2; // Syntax error
            else
            {
              strncpy( command, cmdLine, parameter - cmdLine );
              command[ parameter - cmdLine ] = 0;
              while( * ( ++ parameter ) == ' ' )
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
      client << F("500 Syntax error") << eol;
    }
  }
  return rc;
}

boolean FtpServer::haveParameter()
{
  if( strlen( parameter ) > 0 )
    return true;
  client << "501 No file name" << eol;
  return false;  
}

// Make complete path/name from cwdName and parameter
//
// 3 possible cases: parameter can be absolute path, relative path or only the name
//
// parameter:
//   fullName : where to store the path/name
//
// return:
//    true, if done

boolean FtpServer::makePath( char * fullName, char * param )
{
  if( param == NULL )
    param = parameter;
    
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

  client << F("500 Command line too long") << eol;
  return false;
}

boolean FtpServer::makeExistsPath( char * path, char * param )
{
  if( ! makePath( path, param ))
    return false;
  if( sd.exists( path ))
    return true;
  client << F("550 ") << path << F(" not found.") << eol;
  return false;
}

// Calculate year, month, day, hour, minute and second
//   from first parameter sent by MDTM command (YYYYMMDDHHMMSS)
// Accept longer parameter YYYYMMDDHHMMSSmmm where mmm are milliseconds
//   but don't take in account additional digits
//
// parameters:
//   dt: 15 length string for 14 digits and terminator
//   pyear, pmonth, pday, phour, pminute and psecond: pointer of
//     variables where to store data
//
// return:
//    0 if parameter is not YYYYMMDDHHMMSS
//    length of parameter + space
//
// Date/time are expressed as a 14 digits long string
//   terminated by a space and followed by name of file

uint8_t FtpServer::getDateTime( char * dt, uint16_t * pyear, uint8_t * pmonth, uint8_t * pday,
                                uint8_t * phour, uint8_t * pminute, uint8_t * psecond )
{
  uint8_t i;
  dt[ 0 ] = 0;
  if( strlen( parameter ) < 15 ) //|| parameter[ 14 ] != ' ' )
    return 0;
  for( i = 0; i < 14; i ++ )
    if( ! isdigit( parameter[ i ]))
      return 0;
  for( i = 14; i < 18; i ++ )
    if( parameter[ i ] == ' ' )
      break;
    else if( ! isdigit( parameter[ i ]))
      return 0;
  if( i == 18 )
    return 0;
    
  strncpy( dt, parameter, 14 );
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
  strncpy( dt, parameter, 14 );
  #ifdef FTP_DEBUG
    Serial << F(" File modification time: ") << * pyear << F("/") << * pmonth << F("/") << * pday
           << F(" ") << * phour << F(":") << * pminute << F(":") << * psecond << eol;
  #endif
  return i;
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

