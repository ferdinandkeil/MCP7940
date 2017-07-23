/*******************************************************************************************************************
** Example program for using the MCP7940 library and the adafruit GPS Breakout V3 to calibrate the RTC.           **
**                                                                                                                **
** The MCP7940 library uses the standard SPI Wire library for communications with the RTC chip and has also used  **
** the class definitions of the standard RTClib library from Adafruit/Jeelabs.                                    **
**                                                                                                                **
** The data sheet for the MCP7940M is located at http://ww1.microchip.com/downloads/en/DeviceDoc/20002292B.pdf.   **
** The description for the GPS from adafruit can be found at https://learn.adafruit.com/adafruit-ultimate-gps/.   **
**                                                                                                                **
** Since the GPS uses TTL level serial data it needs a communications port, and the standard port is already in   **
** use for our I/O. The SoftwareSerial library has trouble with the long NMEA sentences, so this sketch is written**
** for Arduino devices that have more than one hardware serial port. This sketch runs on an ATMEGA board.         **
**                                                                                                                **
** This program is free software: you can redistribute it and/or modify it under the terms of the GNU General     **
** Public License as published by the Free Software Foundation, either version 3 of the License, or (at your      **
** option) any later version. This program is distributed in the hope that it will be useful, but WITHOUT ANY     **
** WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the   **
** GNU General Public License for more details. You should have received a copy of the GNU General Public License **
** along with this program.  If not, see <http://www.gnu.org/licenses/>.                                          **
**                                                                                                                **
** Vers.  Date       Developer           Comments                                                                 **
** ====== ========== =================== ======================================================================== **
** 1.0.0  2017-07-23 Arnd@SV-Zanshin.Com Initial coding                                                           **
**                                                                                                                **
*******************************************************************************************************************/
#include <MCP7940.h>                                                          // Include the MCP7940 RTC library  //
#include <Adafruit_GPS.h>                                                     // Library of functions for the GPS //

/*******************************************************************************************************************
** Declare all program constants                                                                                  **
*******************************************************************************************************************/
const uint32_t SERIAL_SPEED            = 115200;                              // Set the baud rate for Serial I/O //
const uint8_t  SPRINTF_BUFFER_SIZE     =     32;                              // Buffer size for sprintf()        //
const uint16_t GPS_BAUD_RATE           =   9600;                              // GPS Module serial port baud rate //
const uint32_t DISPLAY_INTERVAL_MILLIS = 600000;                              // milliseconds between outputs     //

/*******************************************************************************************************************
** Declare global variables and instantiate classes                                                               **
*******************************************************************************************************************/
MCP7940_Class MCP7940;                                                        // Create an instance of the MCP7940//
Adafruit_GPS  GPS(&Serial2);                                                  // Instantiate the GPS on serial 2  //
char          inputBuffer[SPRINTF_BUFFER_SIZE];                               // Buffer for sprintf()/sscanf()    //
/*******************************************************************************************************************
** Method Setup(). This is an Arduino IDE method which is called upon boot or restart. It is only called one time **
** and then control goes to the main loop, which loop indefinately.                                               **
*******************************************************************************************************************/
void setup() {                                                                // Arduino standard setup method    //
  Serial.begin(SERIAL_SPEED);                                                 // Start serial port at Baud rate   //
  Serial.print(F("\nStarting CalibrateFromGPS program\n"));                   // Show program information         //
  Serial.print(F("- Compiled with c++ version "));                            //                                  //
  Serial.print(F(__VERSION__));                                               // Show compiler information        //
  Serial.print(F("\n- On "));                                                 //                                  //
  Serial.print(F(__DATE__));                                                  //                                  //
  Serial.print(F(" at "));                                                    //                                  //
  Serial.print(F(__TIME__));                                                  //                                  //
  Serial.print(F("\n"));                                                      //                                  //
  GPS.begin(GPS_BAUD_RATE);                                                   // Initialize the GPS at baud       //
  GPS.sendCommand(PMTK_SET_NMEA_OUTPUT_RMCGGA);                               // RMC & GGA including altitude     //
  GPS.sendCommand(PMTK_SET_NMEA_OUTPUT_RMCONLY);                              // send RMC (recommended minimum)   //
//GPS.sendCommand(PMTK_SET_NMEA_UPDATE_100_MILLIHERTZ);                       // Data every 10 seconds            //
  GPS.sendCommand(PMTK_API_SET_FIX_CTL_100_MILLIHERTZ);                       // Fixes every 10 seconds           //
  Serial.println("\n- Initializing MCP7940");                                 //                                  //
  while (!MCP7940.begin()) {                                                  // Initialize RTC communications    //
    Serial.println(F("-  Unable to find MCP7940M. Checking again in 3s."));   // Show error text                  //
    delay(3000);                                                              // wait a second                    //
  } // of loop until device is located                                        //                                  //
  Serial.print("- Current TRIM is ");                                         //                                  //
  Serial.print(MCP7940.getCalibrationTrim()*2);                               //                                  //
  Serial.println(" clock cycles per minute");                                 //                                  //
  Serial.println("- Getting GPS fix with Date/Time");                         //                                  //
  DateTime GPStime = readGPS();                                               // returns a valid date/time        //
  Serial.print(F("- Setting MCP7940 using GPS time to "));                    //                                  //
  MCP7940.adjust(GPStime);                                                    //                                  //
  sprintf(inputBuffer,"%04d-%02d-%02d %02d:%02d:%02d", GPStime.year(),        // Use sprintf() to pretty print    //
          GPStime.month(), GPStime.day(), GPStime.hour(), GPStime.minute(),   // date/time with leading zeroes    //
          GPStime.second());                                                  //                                  //
  Serial.println(inputBuffer);                                                // Display the current date/time    //
} // of method setup()                                                        //                                  //
/*******************************************************************************************************************
** Method to read data from the GPS port and then use the Adafruit library to parse the date and time. The GPS    **
** returns a milliseconds portion which is not used by the RTC but since we are doing a time calibration we are   **
** going to round this value up to the next second and pause in the routine for the appropriate amount of time    **
*******************************************************************************************************************/
DateTime readGPS() {                                                          // Read from the GPS                //
  DateTime gpsTime;                                                           // Define the return DateTime       //
  while(true) {                                                               // Loop until we have valid fix     //
    // read a single character from GPS communications port. The read routine buffers the data inside the class   //
    char c = GPS.read();                                                      // Read the next character          //
    if (c) Serial.print(c); // echo the NMEA data to the output port as it arrives for debugging purposes       //
    if (GPS.newNMEAreceived()) {                                              // If we have a new NMEA sentence   //
      if (GPS.parse(GPS.lastNMEA())) {                                        // and it is parseable/legal        //
        if (GPS.fix) {                                                        // and the GPS has a valid fix      //
          delay(1000-GPS.milliseconds);                                       // synchronize to the exact second  //
          gpsTime = DateTime(GPS.year,GPS.month,GPS.day,GPS.hour,GPS.minute,  // Then set a DateTime instance to  //
                    GPS.seconds) + TimeSpan(0,0,0,1);                         // this value plus one second       //
          break;                                                              // Exit this infinite loop now      //
        } // of if-then we have a valid fix                                   //                                  //
      } // of if-then we could parse the sentence data                        //                                  //
    } // of if-then we have new NMEA data                                     //                                  //
  } // loop forever until we force exit                                       //                                  //
  return(gpsTime);                                                            // return our DateTime to caller    //
} // of method readGPS                                                        //                                  //
/*******************************************************************************************************************
** This is the main program for the Arduino IDE, it is an infinite loop and keeps on repeating.                   **
*******************************************************************************************************************/
void loop() {                                                                 //                                  //
  static uint8_t secs;                                                        // store the seconds value          //
  static DateTime RTCTime,GPSTime;                                            // Declare RTC and GPS times        //
  static uint32_t nextDisplayTime = millis()+DISPLAY_INTERVAL_MILLIS;         // 10 Minutes between output        //
  GPSTime = readGPS();                                                        // get the GPS time                 //
  RTCTime = MCP7940.now();                                                    // get the current time             //
  if(GPSTime.second()!=RTCTime.second()) {                                    // just compare the seconds portion //
    Serial.println("Discrepancy of 1 or more seconds detected!");             //                                  //
    sprintf(inputBuffer,"GPS: %04d-%02d-%02d %02d:%02d:%02d", GPSTime.year(), // Use sprintf() to pretty print    //
            GPSTime.month(), GPSTime.day(), GPSTime.hour(), GPSTime.minute(), // date/time with leading zeroes    //
            GPSTime.second());                                                //                                  //
    Serial.println(inputBuffer);                                              // Display the current date/time    //
    sprintf(inputBuffer,"RTC: %04d-%02d-%02d %02d:%02d:%02d", RTCTime.year(), // Use sprintf() to pretty print    //
            RTCTime.month(), RTCTime.day(), RTCTime.hour(), RTCTime.minute(), // date/time with leading zeroes    //
            RTCTime.second());                                                //                                  //
    Serial.println(inputBuffer);                                              // Display the current date/time    //
    Serial.print("Calibration changes trim from ");                           //                                  //
    Serial.print(MCP7940.getCalibrationTrim());                               //                                  //
    Serial.print(" to ");                                                     //                                  //
    Serial.println(MCP7940.calibrate(GPSTime));                               //                                  //
  } // of if-then we have difference                                          //                                  //
  if(millis()>nextDisplayTime) {                                              // Display time once in a while     //
    sprintf(inputBuffer,"%04d-%02d-%02d %02d:%02d:%02d", GPSTime.year(),      // Use sprintf() to pretty print    //
            GPSTime.month(), GPSTime.day(), GPSTime.hour(), GPSTime.minute(), // date/time with leading zeroes    //
            GPSTime.second());                                                //                                  //
    Serial.println(inputBuffer);                                              // Display the current date/time    //
    nextDisplayTime = millis()+DISPLAY_INTERVAL_MILLIS;                       // Set next display time            //
  } // of if display time has elapsed                                         //                                  //
} // of method loop()                                                         //----------------------------------//
