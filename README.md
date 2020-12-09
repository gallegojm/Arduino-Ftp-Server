# ======================
# How to use FtpServer on Arduino
# ======================

Tested with Ide 1.8.13 on Arduino Due and MKR with ethernet module W5100, W5200 or W5500

This is version 2. It has many changes:
   - It must be easier to configure,
   - FatLib and Streaming libraries are not more used 

   - Download and install FtpServer library
   - Edit file FtpServer/src/FtpServerConfig.h and
   - Modify **#define FTP_FILESYST** according to the files system library you are using
   - The possible choices are:

## 1) Library SdFat version 1.4 from William Greiman
   - Link: https://github.com/greiman/SdFat/archive/1.1.4.zip
   - In file FtpServerConfig.h set **FTP_FILESYST** to **FTP_SDFAT1**
   - Make sure that a version 1.4.x of the SdFat library is installed
   - In the Ide, open example FtpServerSdFat1
   - Study the example and modify it according to your hardware
   - Upload it and watch the initialisation steps in the serial monitor 
   - Open a session in a Ftp client (user "arduino", password "test",
       protocol set to normal FTP server (no encryption),
       select single data connection mode when available

## 2) New library SdFat version 2.0.2 from William Greiman
   - Link: https://github.com/greiman/SdFat
   - In file FtpServerConfig.h set **FTP_FILESYST** to **FTP_SDFAT2**
   - Make sure that a version 2.0.2 of the SdFat library is installed
   - This library allows to use the exFat filesystem, in addition to Fat16/32
   - Check the SdFat/src/SdFatConfig.h file and verify that SDFAT_FILE_TYPE has
       the correct value according to the format of your memory card
   - In the Ide, open example FtpServerSdFat2
   - Continue as for SdFat 1.4

## 3) Library FatFs from ChaN adapted by me to run on Arduino
   - Link: https://github.com/gallegojm/Arduino-FatFs
   - For low level access to memory card, need SdFat version 1.4
   - In file FtpServerConfig.h set **FTP_FILESYST** to **FTP_FATFS**
   - In the Ide, open example FtpServerFatFs
   - Continue as for SdFat 1.4

## 4) Libraries Adafruit_SPIFlash and SdFat-Adafruit-Fork to access SPI memories
   - Needs: https://github.com/adafruit/Adafruit_SPIFlash
   - Needs: https://github.com/adafruit/SdFat
   - In file FtpServerConfig.h set **FTP_FILESYST** to **FTP_SPIFM**
   - Check files Adafruit_SPIFlash/src/flash_devices.h and
       Adafruit_SPIFlash/src/Adafruit_SPIFlashBase.cpp to select your
       memory chip
   - In the Ide, open example FtpServerSpiFlash
   - Continue as for SdFat 1.4

## Differences between those libraries:
   - The new version of **SdFat** is the most recommendable as developed and maintained especially for the Arduino. Be sure to use version 2.0.2 or more recent.
   - FatFs lets you select the character encoding and the code page. This is useful if files name include accented letters but this increases the size of the sketch and the memory used.
   - Adafruit_SPIFlash and SdFat-Adafruit-Fork permit to use SPI flash memories instead of SD memories.

# ========
# Definitions
# ========

You may have to modify some of the definitions in FtpServerConfig.h:
 - **FTP_FILESYST** allows to define the files system used
 - **FTP_DEBUG**    if defined, print to the Ide serial monitor information for debugging.
 - **FTP_DEBUG1**   if defined, print additional info
 - **FTP_SERIAL**   lets redirect debug info to an other port than Serial
 - **FTP_TIME_OUT** and **FTP_AUTH_TIME_OUT** are expressed in seconds.
 - **FTP_BUF_SIZE** is the size of the file buffer for read and write operations.
               This size affects the transmission speed. Values of 2048 or 1024 give
               the best speed results, but can be reduced if memory usage is critical.

# ======
# Functions
# ======

## Declaration of the Ftp Server:
 - **FtpServer ftpSrv;**  Default command port to 21 and data port in passive mode to 55600
 - **FtpServer ftpSrv( 421 );** Select command port
 - **FtpServer ftpSrv( 221, 25000 );** Select command and data ports

## Initialization of the FTP server:
 - **ftpSrv.init();**
 - In passive mode, when accessing the server from outside his subnet, it can be
  necessary with some clients to reply them with the server's external ip address
  
  **ftpSrv.init( IPAddress( 11, 22, 33, 44 ));**
  
## Setting of user's credentials
 - Default is 'arduino' for the user name and 'test' for the password.
  
 - This can be changed with:
  **ftpSrv.credentials( "myname", "123" );**
 - Maximum length for name and password is 16
  
## Calling the Ftp Server routine in the loop():
  **ftpSrv.service();**
 - If needed, this function returns the status of the Ftp Server as an 8 bit integer:
  + bits 0-2 represents the stage of the ftp command connexion
             see the definition of **enum ftpCmd** in FtpServer.h
  + bits 3-5 represents the stage of the ftp transfer
             see the definition of **enum ftpTransfer**
  + bits 6 & 7 represents the stage of the data connexion
             see the definition of **enum ftpDataConn**
 - As an example, uncomment the line **#define FTP_DEBUG1** in the file FtpServerConfig.h
             and run the sketch FtpServerStatusLed
       
# ===========
# FTP clients
# ===========

I test FtpServer with those clients:

## Under Windows:
  FTP Rush, Filezilla, WinSCP, NcFTP (and ncftpget, ncftpput), Firefox, command line ftp.exe
  
## Under Ubuntu:
  gFTP, Filezilla, NcFTP(and ncftpget, ncftpput), lftp, ftp, Firefox
  
## Under Android:
  AndFTP, FTP Express, Firefox
  
## With a second Arduino:
  using the sketch of SurferTim at http://playground.arduino.cc/Code/FTP

When available, you have to select single data connection mode.

## FTP Rush:
To force FTP Rush to use the primary connection for data transfers:
Go to Tools/Site Manager, right-select you site and Site Properties
In General, check "Single connection mode"

## WinSCP:
To force WinSCP to use the primary connection for data transfers:
In Login, go to Tools/Preferences.../Transfer/Background,
  set "Maximum number of transfers at the same time" to 1.

## gFTP:
To force gFTP to use the primary connection for data transfers:
Go to FTP/Preferences...,
In General, check "One transfer at a time"
  
## FileZilla:
To force FileZilla to use the primary connection for data transfers:
Go to File/Site Manager then select you site.
In Transfer Settings, check "Limit number of simultaneous connections" and set the maximum to 1

## Firefox:
Enter address ftp://arduino:test@192.168.1.xxx
You can download any file from the server.
You have to quit Firefox to close the connection with the server.

## NcFTP:
This client works on the command line and is perfect for batch processing.
For example:
  ncftpget -d stdout -u arduino -p Due 192.168.1.xxx . /MyDir/MyFile
  to download a file to the current directory
