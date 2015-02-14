How to use FtpServer on Arduino Due and ide 1.6.0

1) Download and install
   - Streaming
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
   
You can use SdFat instead of FatFs
It use less memory but do not allow extended characters in file names.
1) Download SdFat (version for Long File Names if you need them) and install
2) Check configuration parameters in libraries/SdFat/SdFatConfig.h and set these parameters to one:
    #define SD_SPI_CONFIGURATION 1
    #define ENABLE_SPI_TRANSACTION 1
    #define ENABLE_SPI_YIELD 1
3) Set parameter FAT_SYST to 0 in libraries/FatLib/FatLib.h
4) Run libraries/examples/FtpServerTest

On Arduino Mega 2560 you have to use SdFat
