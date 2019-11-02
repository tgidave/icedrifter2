/*!                                                                              
 *  @file icedrifter2.cpp                                                  
 *                                                                               
 *  @mainpage Code to implement the Icedrifter bouy.                     
 *                                                                               
 *  @section intro_sec Introduction                                              
 *  
 *  This code implements functionality that gathers GPS location data,
 *  temperature and air pressure data, and optionally, a remote
 *  temperature probe and light and temperature chain data.  This data
 *  is then sent back to the user using the Iridium system on a daily
 *  or hourly basis.
 *                                                                               
 *  @section author Author                                                       
 *                                                                               
 *  Uncle Dave                                                  
 *                                                                               
 *  @section license License                                                     
 *                                                                               
 *  Unknown (Talk to Cy)                                                        
 *                                                                               
 *  @section HISTORY                                                             
 *                                                                               
 *  v2.0 - First release                                                         
 */

#include <Arduino.h>
#include <LowPower.h>
#include <time.h>
#include <avr/wdt.h>
#include <TinyGPS++.h> // NMEA parsing: http://arduiniana.org
#include <PString.h> // String buffer formatting: http://arduiniana.org

#include "icedrifter2.h"
#include "gps.h"
#include "bmp280.h"
#include "ds18b20.h"
#include "chain.h"
#include "rockblock.h"
#include "serialmux.h"

#define CONSOLE_BAUD 115200

bool firstTime;   /// Set true in the setup function and set false after the first
                  /// time through the loop function.  Used to indicate when to
/// capture the last boot date and time.

bool gotFullFix;  /// Indicates that a Full fix was received.

int noFixFoundCount;  /// Number of times the GPS device could not get a fix.

int fixFound; /// indicates weather the last call to the GPS system returned a fix.

icedrifterData idData;  /// Structure for accumulating and sending sensor data,

//iceDrifterChunk idcChunk;

time_t lbTime;  /// Time and date of the last boot.

// This table is used to determine when to report data through the
// Iridium system.  It is set up as UTC times from midnight to 23:00
// hours.  If an hour is set to false, the system will not report.
// If an hour is set to true, a report will be attempted on the next
// half hour.  I.E. if Noon UTC is set to true, the system will try
// to report at 12:30 UTC.  If all hour entries are set to true the
// system will try to report every hour on the half hour.  If all hour
// entries are set to false, the system will never try to report.
//
// This table is set for standard time and does not account for local
// daylight savings time.

const bool timeToReport[24] = {
  false,  // Midnight UTC
  false,  // 01:00 UTC
  false,  // 02:00 UTC
  false,  // 03:00 UTC
  false,  // 04:00 UTC
  false,  // 05:00 UTC
  false,  // 06:00 UTC
  true,   // 07:00 UTC - Midnight Mountain standard time
  false,  // 08:00 UTC
  false,  // 09:00 UTC
  false,  // 10:00 UTC
  false,  // 11:00 UTC
  false,  // Noon UTC
  false,  // 13:00 UTC
  false,  // 14:00 UTC
  false,  // 15:00 UTC
  false,  // 16:00 UTC
  false,  // 17:00 UTC
  false,  // 18:00 UTC
  true,   // 19:00 UTC - Noon Mountain standard time
  false,  // 20:00 UTC
  false,  // 21:00 UTC
  false,  // 22:00 UTC
  false,  // 23:00 UTC
};

/*
enum period_t { /// Values for setting the watchdog timer.
  SLEEP_15MS,
  SLEEP_30MS,
  SLEEP_60MS,
  SLEEP_120MS,
  SLEEP_250MS,
  SLEEP_500MS,
  SLEEP_1S,
  SLEEP_2S,
  SLEEP_4S,
  SLEEP_8S,
  SLEEP_FOREVER
};
*/

const char hexchars[] = "0123456789ABCDEF";

void printHexChar(uint8_t x) {
  Serial.print(hexchars[(x >> 4)]);
  Serial.print(hexchars[(x & 0x0f)]);
} 

/// powerDown - Put processor into low power mode.
///
/// This function first set up the watchdog timer to go of after
/// the maxiuum interval
/// of 8 seconds and then puts the processor into low power sleep node.  After
/// approximately 8 seconds the interval time will expire and wake up the processor
/// and the program continues.
///
/// \param void
/// \return void
/*
void powerDown(void) {
  ADCSRA &= ~(1 << ADEN);
  wdt_enable(SLEEP_8S);
  WDTCSR |= (1 << WDIE);
  sleepMode(SLEEP_POWER_SAVE);
  sleep();
  noSleep();
}


/// <summary>
/// ISR - Interrupt Service Routine for handeling the watchdog
/// timer interupt.  This routine disables the WDT interupt and
/// then returns. </summary>
/// <param> WDT_vect </param>
/// <returns>"Nothing"</returns>

ISR(WDT_vect) {
  /// WDIE & WDIF is cleared in hardware upon entering this ISR
  wdt_disable();
}
*/
/// <summary>
/// Accumulate and send data. This function captures the sender
/// data and sends that data to the user.
/// </summary>
/// <param name="void"></param>
/// <returns>"void"</returns>

void accumulateAndSendData(void) {

  int i, j;

  int chainRetryCount;
  int totalDataLength; 
  int recCount;
  int cdError;
  uint8_t * wkPtr; 



  idData.idSwitches = idData.idTempSensorCount = idData.idLightSensorCount = 0; 

#ifdef PROCESS_REMOTE_TEMP_SWITCH
  idData.idSwitches |= PROCESS_REMOTE_TEMP_SWITCH;
#endif

#ifdef PROCESS_CHAIN_DATA
  idData.idSwitches |= PEOCESS_CHAIN_DATA_SWITCH;
  idData.idTempSensorCount = TEMP_SENSOR_COUNT;
  idData.idLightSensorCount = LIGHT_SENSOR_COUNT;
#endif 

#ifdef SEND_RGB
  idData.idSwitches |= SEND_RGB_SWITCH;
#endif

//  idData.idRecordType[0] = 'I';
//  idData.idRecordType[1] = 'D';
//  idData.idRecordType[2] = '0';
//  idData.idRecordType[3] = '0';

  idData.idLastBootTime = lbTime;

  if ((fixFound = gpsGetFix(FIX_FULL, &idData)) == false) {
    idData.idGPSTime = 0;
    idData.idLatitude = 0;
    idData.idLongitude = 0;
    idData.idAltitude = 0;
    idData.idSpeed = 0;
    idData.idCourse = 0;
  }

  getBMP280Data(&idData);

#ifdef PROCESS_REMOTE_TEMP
  getRemoteTemp(&idData);
#else
  idData.idRemoteTemp = 0;
#endif // PROCESS_REMOTE_TEMP

  cdError = 0;
#ifdef PROCESS_CHAIN_DATA
  chainRetryCount = 0;

  while ((cdError = processChainData((uint8_t *)&idData.idChainData)) != 0) {
    ++chainRetryCount;
    if (chainRetryCount >= MAX_CHAIN_RETRIES) {
      break;
    }
  }

#endif // PROCESS_CHAIN_DATA
  idData.idcdError = cdError;
  totalDataLength = BASE_RECORD_LENGTH;

#ifdef PROCESS_CHAIN_DATA
  if (cdError == 0) {
    totalDataLength += (TEMP_DATA_SIZE + LIGHT_DATA_SIZE);
  }
#endif  // PROCESS_CHAIN_DATA

  wkPtr = (uint8_t *)&idData; 

#ifdef SERIAL_DEBUG
  DEBUG_SERIAL.print(F("Dumping data record\r\n"));
  DEBUG_SERIAL.print(F("Address = "));
  DEBUG_SERIAL.print((uint32_t)wkPtr, HEX);
  DEBUG_SERIAL.print(F(" size = "));
  DEBUG_SERIAL.print(totalDataLength);
  DEBUG_SERIAL.print(F("\r\n"));

  for (i = 0; i < totalDataLength; i++) {
    printHexChar((uint8_t)*wkPtr);
    ++wkPtr;
  }

  DEBUG_SERIAL.print(F("\r\n"));
#endif
/*
  recCount = 0;
  idPtr = (uint8_t *)&idData;
  chunkPtr = (uint8_t*)&idcChunk.idcBuffer;

  while (totalDataLength > 0) {
    idcChunk.idcSendTime = idData.idGPSTime;
    idcChunk.idcRecordType[0] = 'I';
    idcChunk.idcRecordType[1] = 'D';
    idcChunk.idcRecordNumber = recCount;

    if (totalDataLength > MAX_CHUNK_LENGTH) {
      chunkLength = (MAX_CHUNK_LENGTH + CHUNK_HEADER_SIZE);
      totalDataLength -= MAX_CHUNK_LENGTH;
    } else {
      chunkLength = (totalDataLength + CHUNK_HEADER_SIZE);
      totalDataLength = 0;
    }

    memmove(chunkPtr, idPtr, chunkLength);
    ++recCount;
    idPtr += chunkLength;

#ifdef NEVER_TRANSMIT
    DEBUG_SERIAL.print(F("Transmission disabled by NEVER_TRANSMIT switch.\r\n"));
#else
    rbTransmitIcedrifterData(&idcChunk) chunkLength + CHUNK_HEADER_SIZE);
#endif // NEVER_TRANSMIT
  }
*/
  rbTransmitIcedrifterData(&idData, totalDataLength);
/*
#ifdef SERIAL_DEBUG
  DEBUG_SERIAL.print(F("Sending this data:\r\n"));
  DEBUG_SERIAL.print(F("idData.idRecordType   = "));
  DEBUG_SERIAL.print(idData.idRecordType[0]);
  DEBUG_SERIAL.print(idData.idRecordType[1]);
  DEBUG_SERIAL.print(idData.idRecordType[2]);
  DEBUG_SERIAL.print(idData.idRecordType[3]);
  DEBUG_SERIAL.print(F("\r\nidData.idLastBootTime = "));
  DEBUG_SERIAL.print(idData.idLastBootTime);
  DEBUG_SERIAL.print(F("\r\nidData.idGPSTime      = "));
  DEBUG_SERIAL.print(idData.idGPSTime);
  DEBUG_SERIAL.print(F("\r\nidData.Latitude       = "));
  DEBUG_SERIAL.print(idData.idLatitude);
  DEBUG_SERIAL.print(F("\r\nidData.idLongitude    = "));
  DEBUG_SERIAL.print(idData.idLongitude);
  DEBUG_SERIAL.print(F("\r\nidData.idAltitude     = "));
  DEBUG_SERIAL.print(idData.idAltitude);
  DEBUG_SERIAL.print(F("\r\nidData.idSpeed        = "));
  DEBUG_SERIAL.print(idData.idSpeed);
  DEBUG_SERIAL.print(F("\r\nidData.idCourse       = "));
  DEBUG_SERIAL.print(idData.idCourse);
  DEBUG_SERIAL.print(F("\r\nidData.idTemperature  = "));
  DEBUG_SERIAL.print(idData.idTemperature);
  DEBUG_SERIAL.print(F("\r\nidData.idPressure     = "));
  DEBUG_SERIAL.print(idData.idPressure);
  DEBUG_SERIAL.print(F("\r\nidData.idRemoteTemp   = "));
  DEBUG_SERIAL.print(idData.idRemoteTemp);
  DEBUG_SERIAL.print(F("\r\n"));
#endif

#ifdef NEVER_TRANSMIT
  DEBUG_SERIAL.print(F("Transmission disabled by NEVER_TRANSMIT switch.\r\n"));
#else
  rbTransmitIcedrifterData(&idData, sizeof(idData));
#endif
*/

}

//! setup - This is an arduino defined routine that is called only once after the processor is booted.
//!
void setup() {

  pinMode(muxINHPort, OUTPUT);
  pinMode(muxAPort, OUTPUT);
  pinMode(muxBPort, OUTPUT);

  serialMuxInit();
  serialMuxSetToOff();

  pinMode(GPS_POWER_PIN, OUTPUT);
  digitalWrite(GPS_POWER_PIN, LOW);

  pinMode(BMP280_POWER_PIN, OUTPUT);
  digitalWrite(BMP280_POWER_PIN, LOW);

#ifdef PROCESS_REMOTR_TEMP
  pinMode(DS18B20_POWER_PIN, OUTPUT);
  digitalWrite(DS18B20_POWER_PIN, LOW);
#endif

#ifdef PROCESS_CHAIN_DATA
  pinMode(CHAIN_POWER_PIN, OUTPUT);
  digitalWrite(CHAIN_POWER_PIN, LOW);
#endif

  pinMode(ROCKBLOCK_POWER_PIN, OUTPUT);
  digitalWrite(ROCKBLOCK_POWER_PIN, LOW);

#ifdef SERIAL_DEBUG
  //! Start the serial ports
  DEBUG_SERIAL.begin(CONSOLE_BAUD);
#endif

  gotFullFix = false; //! Clear the GPS full fix switch so the first call to the loop function requests a full fix.
  firstTime = true;

#ifdef SERIAL_DEBUG
  DEBUG_SERIAL.print(F("Setup done\r\n")); //! Let the user know we are done with the setup function.
#endif

}

//! loop - This is the main processing function for the arduino system.
//!
//! The first time through this function a full GPS fix is requested.  If no fix is
//! received the processor is put to sleep for 60 minutes and then a full GPS fix
//! will be requested again.  This continues until a full fix is received.
//!
//! Upon receiving a full GPS fix. the minutes are calculated to wake up the processor
//! at the next half hour and the processor is put to sleep.
//!
//! Once a full GPS fix is received, only the current time is requested from the GPS.
//! That's all that is needed to calculate the minutes to the next wake up time.
void loop() {

  int sleepSecs;  //! Number of seconds to sleep before the processor is woken up.
  int sleepMins;  //! Number of minutes to sleep before the processor is woken up.

  noFixFoundCount = 0;  //! clear the no fix found count.

  //! Check to see if a full fix was received.  If not, try to get a full fix.
  //! If so, just get a time fix.
  if (gotFullFix) {
    fixFound = gpsGetFix(FIX_TIME, &idData);
  } else {
    fixFound = gpsGetFix(FIX_FULL, &idData);
  }

  //! If a GPS fix was received, set the gotFullFix switch and clear the noFixFound count.
  //! Otherwise add one to the noFixFoundCount.
  if (fixFound) {
    gotFullFix = true;
    noFixFoundCount = 0;
    if (firstTime) {
      lbTime = idData.idLastBootTime = idData.idGPSTime;
    }
  } else {
    ++noFixFoundCount;
  }

#ifdef TEST_ALL
  accumulateAndSendData();
#else
  if (!firstTime &&
      ((fixFound && timeToReport[hour(idData.idGPSTime)] == true) ||
       noFixFoundCount >= 24)) {
    noFixFoundCount = 0;
    accumulateAndSendData();
  }
#endif

  // Accumulating and sending the data can take a while so update the time again.
  fixFound = gpsGetFix(FIX_TIME, &idData);
  firstTime = false;

  // If a GPS fix was found
  if (fixFound) {
    // Calculate the minutes until the next half hour,
    sleepMins = 90 - minute(idData.idGPSTime);
    // If it less than 15 minutes until the nex half hour,
    if (sleepMins >= 75) {
      sleepMins -= 60;
    }

#ifdef SERIAL_DEBUG
    DEBUG_SERIAL.print(F("Fix found - sleep "));
    DEBUG_SERIAL.print(sleepMins);
    DEBUG_SERIAL.print(F(" minutes\r\n"));
    DEBUG_SERIAL.flush();
    DEBUG_SERIAL.end();
#endif
    sleepSecs = sleepMins * 60;
  } else {
#ifdef SERIAL_DEBUG
    DEBUG_SERIAL.print(F("Fix not found - sleep 60 minutes\r\n"));
    DEBUG_SERIAL.flush();
    DEBUG_SERIAL.end();
#endif
    sleepSecs = 3600;
  }

  do {
    LowPower.powerDown(SLEEP_8S, ADC_OFF, BOD_OFF);
    sleepSecs -= 8;
  } while (sleepSecs > 0);

#ifdef SERIAL_DEBUG
  DEBUG_SERIAL.begin(CONSOLE_BAUD);
  DEBUG_SERIAL.print(F("wake up\r\n"));
  DEBUG_SERIAL.flush();
#endif
}
