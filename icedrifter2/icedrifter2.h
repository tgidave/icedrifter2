/*!                                                                              
 *  @file icedrifter2.h                                                  
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
 *  v1.0 - First release                                                         
 */                                                                              

#ifndef _ICEDRIFTER2_H
#define _ICEDRIFTER2_H

#ifdef ARDUINO
  #include <TimeLib.h>
  #include <Time.h>
#endif // ARDUINO

// The TEST_ALL switch will will collect and send data at bootup
// and then every hour on the half hour after that.  Comment out
// the next line to run normally.

#define TEST_ALL  // test as much code a possible at bootup.

//#define NEVER_TRANSMIT  // Do everything except transmit data.

//Common power pin that controls power to the BMP280, the DS18B20,
//and the temperature and light chains.

#define COMMON_POWER_PIN 20

//To turn off the debugging messages, comment out the next line.

#define SERIAL_DEBUG

//The following defines are used to control what data is transmitted during debugging.
//If "SERIAL_DEBUG" is not defined they have no effect.

#ifdef SERIAL_DEBUG
#define DEBUG_SERIAL Serial
#define DEBUG_BAUD 115200

#define SERIAL_DEBUG_GPS
#define SERIAL_DEBUG_BMP280
#define SERIAL_DEBUG_DS18B20
#define SERIAL_DEBUG_CHAIN
#define SERIAL_DEBUG_ROCKBLOCK
#endif // SERIAL_DEBUG

#define PROCESS_REMOTE_TEMP

//#define PROCESS_CHAIN_DATA

#define MAX_CHAIN_RETRIES 3
#define TEMP_SENSOR_COUNT   16
#define LIGHT_SENSOR_COUNT  8
#define LIGHT_SENSOR_FIELDS 4
#define TEMP_DATA_SIZE (TEMP_SENSOR_COUNT * sizeof(uint16_t))
#define LIGHT_DATA_SIZE ((LIGHT_SENSOR_COUNT * LIGHT_SENSOR_FIELDS) * sizeof(uint16_t))

typedef struct chainData {
  uint16_t cdTempData[TEMP_SENSOR_COUNT];
  uint16_t cdLigntData[LIGHT_SENSOR_COUNT][LIGHT_SENSOR_FIELDS];
} chainData;


//icedrifter data record definition.
typedef struct icedrifterData {
  uint8_t idSwitches;

#define PROCESS_REMOTE_TEMP_SWITCH  0x01
#define PEOCESS_CHAIN_DATA_SWITCH   0x02

  uint8_t idTempSensorCount; 
  uint8_t idLightSensorCount;
  uint8_t idcdError;
#ifdef ARDUINO
  time_t idLastBootTime;
  time_t idGPSTime; 
#else
  uint32_t idLastBootTime;
  uint32_t idGPSTime; 
#endif // ARDUINO
  float idLatitude;
  float idLongitude;
  float idAltitude;
  float idSpeed;
  float idCourse;
  float idTemperature;
  float idPressure;
  float idRemoteTemp;

#define BASE_RECORD_LENGTH  44 

#ifdef PROCESS_CHAIN_DATA
  chainData idChainData;
#endif // PROCESS_CHAIN_DATA

} icedrifterData; 
/*
#define MAX_CHUNK_LENGTH 332
#define CHUNK_HEADER_SIZE 8

typedef struct iceDrifterChunk {
  time_t idcSendTime;
  char idcRecordType[2];
  uint16_t idcRecordNumber;
  uint8_t idcBuffer[MAX_CHUNK_LENGTH];
} iceDrifterChunk;
*/

#endif // _ICEDRIFTER2_H
