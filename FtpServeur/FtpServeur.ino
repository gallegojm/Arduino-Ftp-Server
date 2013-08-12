
/*
 * FTP Serveur for Arduino Due and Ethernet shield (W5100) or WIZ820io (W5200)
 * Copyright (c) 2013 by Jean-Michel Gallego
 * 
 * Use Streaming.h from Mial Hart
 * Use SdFat.h from William Greiman
 *   with extension for long names (see http://forum.arduino.cc/index.php?topic=171663.0 )
 * Use Ethernet library with somes modifications:
 *   need to add some functions (see http://forum.arduino.cc/index.php?topic=182354.0 )
 *   modification for WIZ820io (see http://forum.arduino.cc/index.php?topic=139147.0 
 *     and https://github.com/jbkim/W5200-Arduino-Ethernet-library )
 * 
 * Actual limitations:
 *   Not implemented: 
 *     PASS, ACCT
 *     STOR, STOU, APPE
 *     RNFR, RNTO, DELE
 *     MKD, RMD
 *     SMNT, REIN, PASV, ALLO, REST, ABOR
 *     SITE, SYST, STAT, HELP
 *   Poorly implemented:
 *     LIST
 *
 * Tested with those clients:
 *  FTP Rush : ok
 *  Filezilla: problem whit RETR
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

char version[] = "2013-08-11";

// Define Chip Select for your SD card according to hardware 
#define CS_SDCARD 9
//  #define CS_SDCARD 4

// Mac address of ethernet adapter
byte mac[] = { 0x90, 0xa2, 0xda, 0x00, 0x00, 0x00 };
// IP address of FTP server
IPAddress serverIp( 192, 168, 1, 111 );
// IP address of client for data
IPAddress dataIp( 0, 0, 0, 0 );
EthernetServer ftpServer( 21 );
EthernetClient client;
EthernetClient data;
uint16_t dataPort = 20;

#define CMD_SIZE 256        // max size of a command
#define CWD_SIZE 256        // max size of a directory name
#define BUF_SIZE 512        // size of file buffer for read/write

char cmdLine[ CMD_SIZE ];   // where to store incoming char from client
char cwdLName[ CWD_SIZE ];  // long name for current directory
char cwdSName[ CWD_SIZE ];  // short name for current directory
char command[ 5 ];          // command sent by client
char * parameters;          // point to begin of parameters sent by client
uint16_t iCL;               // pointer to cmdLine next incoming char


/*******************************************************************************
**                                                                            **
**                               INITIALISATION                               **
**                                                                            **
*******************************************************************************/

void setup()
{
  WDT_Disable( WDT );
  
  Serial.begin(9600);
  Serial << "Initiating Ftp Server on Due" << endl;

  // Initialize the SdCard.
  if( ! sd.begin( CS_SDCARD, SPI_HALF_SPEED ))
    sd.initErrorHalt();
  strcpy( cwdLName, "/" );
  strcpy( cwdSName, "/" );
  if( ! sd.chdir( cwdLName ))
    sd.errorHalt( "sd.chdir" );
  pinMode( CS_SDCARD, OUTPUT );
  digitalWrite( CS_SDCARD, HIGH );

  // Initialize the network
  Ethernet.begin( mac, serverIp );
  // Tells the ftp server to begin listening for incoming connection
  ftpServer.begin();

  delay( 2000 );

  Serial << "Ftp server waiting for connection on port 21." << endl;
}

/*******************************************************************************
**                                                                            **
**                                 MAIN LOOP                                  **
**                                                                            **
*******************************************************************************/

void loop()
{
  int8_t rc;
  uint32_t mTimeOut = 5 * 60 * 1000;  // disconnect after 5 min of inactivity
  uint32_t mEndConnection;

  // wait for a client to connect
  client = ftpServer.connected();
  if( client == true )
  {
    Serial << "Client connected!" << endl;
    client << "220---Welcome to FTP for Arduino---\r\n";
    client << "220 Version " << version << "\r\n";

    // wait for client sending user name for 30 seconds
    iCL = 0;
    mEndConnection = millis() + 30;
    do
    {
      if( client.available())
        rc = readChar();
    }
    while( client.isConnected() && rc != 0 && ((int32_t) ( mEndConnection - millis() ) > 0 ));

    //rc = readCmd();
    if( rc == -1 )
      client << "530 Timeout\r\n";
    else if( rc == -2 || strcmp( command, "USER" ))
      client << "500 Syntax error\r\n";
    else if( strcmp( parameters, "arduino" ))
      client << "530 \r\n";
    else
    {
      Serial << "OK. Waiting for commands." << endl;
      client << "230 OK.\r\n";
      strcpy( cwdLName, "/" );
      strcpy( cwdSName, "/" );

      iCL = 0;
      mEndConnection = millis() + mTimeOut;
      while( client.isConnected() && ((int32_t) ( mEndConnection - millis() ) > 0 ))
      {
        if( client.available())
        {
          rc = readChar();
          if( rc == -2 )
          {
            client << "500 Don't understand " << cmdLine << "\r\n";
            iCL = 0;
          }
          if( rc >= 0 )
          {
            /*
              CDUP - Change to Parent Directory 
            */
            if( ! strcmp( command, "CDUP" ))
            {
              if( strlen( cwdLName ) > 1 )
              {
                char * pSep = strrchr( cwdLName, '/' );
                if( pSep > cwdLName )
                {
                  char tmp[ CWD_SIZE ];
                  char shortPath[ CWD_SIZE ];
                  strcpy( tmp, cwdLName );
                  tmp[ pSep - cwdLName + 1 ] = 0;
                  boolean ok = l2sPath( shortPath, tmp, CWD_SIZE );
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
            /*
              CWD - Change Working Directory
            */
            if( ! strcmp( command, "CWD" ))
            {
              if( ! strcmp( parameters, "." ))  // 'CWD .' is the same as PWD command
                client << "257 \"" << cwdLName << "\" is your current directory\r\n";
              else
              {
                boolean ok = true;
                char tmp[ CWD_SIZE ];
                char shortPath[ CWD_SIZE ];
                if( strlen( parameters ) == 0 ) // no parameters. go to root
                {
                  strcpy( cwdLName, "/" );
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
                  ok = l2sPath( shortPath, tmp, CWD_SIZE ); // compute short name of dir
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
            /*
              FEAT - New Features
            */
            else if( ! strcmp( command, "FEAT" ))
            {
              client << "211-Extensons suported:\r\n";
              client << " MLSD\r\n";
              client << "211 End.\r\n";
            }
            /*
              MLSD - Listing for Machine Processing (see RFC 3659)
            */
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
            /*
              NLST - Name List 
              weak implementation of LIST
            */
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
            /*
              MODE - Transfer Mode 
            */
            else if( ! strcmp( command, "MODE" ))
            {
              if( ! strcmp( parameters, "S" ))
                client << "200 S Ok\r\n";
              // else if( ! strcmp( parameters, "B" ))
              //  client << "200 B Ok\r\n";
              else
                client << "504 Only S(tream) is suported\r\n";
            }
            /*
              NOOP
            */
            else if( ! strcmp( command, "NOOP" ))
            {
              dataPort = 0;
              client << "200 Zzz...\r\n";
            }
            /*
              PORT - Data Port
            */
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
            /*
              PWD - Print Directory
            */
            else if( ! strcmp( command, "PWD" ))
              client << "257 \"" << cwdLName << "\" is your current directory\r\n";
            /*
              QUIT
            */
            else if( ! strcmp( command, "QUIT" ))
            {
              client << "221 Goodbye\r\n";
              break;
            }
            /*
              RETR - Retrieve
            */
            else if( ! strcmp( command, "RETR" ))
            {
              if( ! data.connect( dataIp, dataPort ))
                client << "425 No data connection\r\n";
              else
              {
                char fullFileName[ 252 ];
                if( ! l2sName( fullFileName , cwdSName, parameters ))
                  client << "550 No such file " << parameters << "\r\n";
                else
                {
                  SdFile file;
                  if( ! file.open( fullFileName, O_READ ))
                    client << "450 Can't open " << fullFileName << "\r\n";
                  else
                  {
                    uint8_t buf[ BUF_SIZE ];
                    int16_t nb;
                    uint32_t fSize = file.fileSize();
                    client << "150-Accepted data connection\r\n";
                    client << "150 " << fSize << " bytes to download\r\n";
                    uint32_t millisBeginTrans = millis();
                    do
                    {
                      nb = file.read( buf, BUF_SIZE );
                      if( nb > 0 )
                        data.write( buf, nb );
                    }
                    while( nb > 0 );
                    file.close();
                    uint32_t deltaT = (int32_t) ( millis() - millisBeginTrans );
                    client << "226-File successfully transferred\r\n";
                    client << "226 " << deltaT << " ms, "
                           << fSize / deltaT << " kbytes/s\r\n";
                    file.close();
                  }    
                  data.stop();
                }
              }
            }
            /*
              STRU - File Structure
            */
            else if( ! strcmp( command, "STRU" ))
            {
              if( ! strcmp( parameters, "F" ))
                client << "200 F Ok\r\n";
              // else if( ! strcmp( parameters, "R" ))
              //  client << "200 B Ok\r\n";
              else
                client << "504 Only F(ile) is suported\r\n";
            }
            /*
              TYPE - Data Type
            */
            else if( ! strcmp( command, "TYPE" ))
            {
              if( ! strcmp( parameters, "A" ))
                client << "200 TYPE is now ASII\r\n";
              // else if( ! strcmp( parameters, "I" ))
              //   client << "200 TYPE is now 8-bit binary\r\n";
              else
                client << "504 Unknow TYPE\r\n";
            }
            /*
              Unrecognized commands ...
            */
            else
            {
              client << "500 Unknow command\r\n";
            }
            mEndConnection = millis() + mTimeOut;
            iCL = 0;
          }
        }
      }
    }
    Serial << " Disconnecting client" << endl;
    delay( 1000 );
    client.stop();
    Serial << "Ftp server waiting for connection on port 21." << endl;
  }
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

int8_t readChar()
{
  int8_t rc = -1;

  char c = client.read();
  Serial << c;
  if( c == '\r' )
    return -1;
  if( c != '\n' )
  {
    if( iCL < CMD_SIZE )
      cmdLine[ iCL ++ ] = c;
    else
      return -2; //  Line too long
    return -1;
  }
  cmdLine[ iCL ] = 0;
  command[ 0 ] = 0;
  parameters = NULL;
  // empty line?
  if( iCL == 0 )
    return 0;
  char * pSpace = strchr( cmdLine, ' ' );
  if( pSpace != NULL )
  {
    if( pSpace - cmdLine > 4 )
      return -2; // Syntax error
    else
    {
      strncpy( command, cmdLine, pSpace - cmdLine );
      command[ pSpace - cmdLine ] = 0;
      uint8_t l = strlen( command );
      for( uint8_t i = 0 ; i < l; i ++ )
        command[ i ] = toupper( command[ i ] );
      parameters = pSpace + 1;
    }
  }
  else if( strlen( cmdLine ) > 4 )
    return -2; // Syntax error
  else
  {
    strcpy( command, cmdLine );
  }
  return iCL;
}

