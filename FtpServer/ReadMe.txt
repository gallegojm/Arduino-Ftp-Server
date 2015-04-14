=================================================
How to use FtpServer on Arduino Due and ide 1.6.0
=================================================

1) Download and install
   - Streaming ( http://arduiniana.org/Streaming/Streaming5.zip )
   - FatFs
2) To test the access to the SD card is ok :
   - load example libraries/FatFs/examples/FatFsDemo,
   - verify that SD_CS_PIN has the correct value (Chip Select for SD card reader),
   - run
3) Download and install
   - FatLib
   - FtpServer
4) Download EthernetServerConnected and overwrite the 2 files in libraries/Ethernet/src
5) To test Ftp Server:
   - restart ide
   - load libraries/examples/FtpServerTest,
   - verify that CS_SDCARD has the correct value (Chip Select for SD card reader),
   - run
   - anotate in the ide serial monitor the IP your router assign to the Ftp Server
   - open a session in a Ftp client (FTPRush, passive mode, user "arduino", password "Due")
   
==================================
You can use SdFat instead of FatFs
==================================

It use less memory but do not allow extended characters in file names.
1) Download SdFat ( https://github.com/greiman/SdFat-beta ) and install
2) Check configuration parameters in libraries/SdFat/SdFatConfig.h and set these parameters to one:
    #define SD_SPI_CONFIGURATION 1
    #define ENABLE_SPI_TRANSACTION 1
    #define ENABLE_SPI_YIELD 1
3) Comment out lines 40-46 in file libraries/SdFat/utility/iostream.h
    to avoid double definition of endl
4) Set parameter FAT_SYST to 0 in libraries/FatLib/FatLib.h
5) Run libraries/examples/FtpServerTest

====================
On Arduino Mega 2560
====================

You have to use SdFat

===============
FTP Rush client
===============

To force FTP Rush to use the primary connection for data transfers:
Go to Tools/Site Manager, right-select you site and Site Properties
In General, check "Single connection mode"

You may have to adjust the Time Zone.

================
FileZilla client
================

To force FileZilla to use the primary connection for data transfers:
Go to File/Site Manager then select you site.
In Transfer Settings, check "Limit number of simultaneous connections" and set the maximum to 1

