/*
 * FTP Serveur for Arduino Due, Arduino MKR
 * and Ethernet shield W5100, W5200 or W5500
 * ( or for Esp8266 with external SD card or SpiFfs ) **
 * Copyright (c) 2014-2020 by Jean-Michel Gallego
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
 **                         SETTINGS FOR FTP SERVER                            **
 **                                                                            **
 *******************************************************************************/

#ifndef FTP_SERVER_CONFIG_H
#define FTP_SERVER_CONFIG_H

// Files system
#define FTP_SDFAT1 0 // Library SdFat version 1.4.x
#define FTP_SDFAT2 1 // Library SdFat version >= 2.0.2
#define FTP_SPIFM  2 // Libraries Adafruit_SPIFlash and SdFat-Adafruit-Fork
#define FTP_FATFS  3 // Library FatFs
// Select one of the previous files system
#define FTP_FILESYST FTP_SDFAT2


// Uncomment to print debugging info to console attached to Arduino
#define FTP_DEBUG


// Uncomment to print additional info
//#define FTP_DEBUG1


// Redirect debug info to console or other port
#define FTP_SERIAL Serial
// #define FTP_SERIAL SerialUSB


// Disconnect client after 5 minutes of inactivity (expressed in seconds)
#define FTP_TIME_OUT  5 * 60 


// Wait for authentication for 10 seconds (expressed in seconds)
#define FTP_AUTH_TIME_OUT 10


// Size of file buffer for read/write
// Transfer speed depends of this value
// Best value depends on many factors: SD card, client side OS, ... 
// But it can be reduced to 512 if memory usage is critical.
#define FTP_BUF_SIZE 2048 //1024 // 512  


#endif // FTP_SERVER_CONFIG_H
