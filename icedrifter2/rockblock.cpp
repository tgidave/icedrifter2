#include <Arduino.h>
#include <IridiumSBD.h>
//#include <SoftwareSerial.h>

#include "icedrifter2.h"
#include "rockblock.h"
#include "serialmux.h"

#define rbSerial Serial1
//SoftwareSerial rbSerial(ROCKBLOCK_RX_PIN, ROCKBLOCK_TX_PIN);

IridiumSBD isbd(rbSerial, ROCKBLOCK_SLEEP_PIN);

iceDrifterChunk idcChunk;

#ifdef SERIAL_DEBUG_ROCKBLOCK
void ISBDConsoleCallback(IridiumSBD *device, char c);
void ISBDDiagsCallback(IridiumSBD *device, char c);
#endif

void rbTransmitIcedrifterData(icedrifterData* idPtr, int idLen) {

  int rc;
  int recCount;
  int wkLen;
  int chunkLen;
  uint8_t* wkPtr;
  uint8_t* chunkPtr;

  wkLen = idLen; 

  // Point the serial mux to the rockblock.
  serialMuxSetToRockBlock();

  // Setup the RockBLOCK
  isbd.setPowerProfile(IridiumSBD::USB_POWER_PROFILE);

#ifdef SERIAL_DEBUG_ROCKBLOCK
  DEBUG_SERIAL.flush();
  DEBUG_SERIAL.println(F("Powering up RockBLOCK\r\n"));
  DEBUG_SERIAL.flush();
#endif

  digitalWrite(ROCKBLOCK_POWER_PIN, HIGH);
  delay(1000);
  rbSerial.begin(ROCKBLOCK_BAUD);
  // Step 3: Start talking to the RockBLOCK and power it up
#ifdef SERIAL_DEBUG_ROCKBLOCK
  DEBUG_SERIAL.flush();
  DEBUG_SERIAL.println(F("RockBLOCK begin\r\n"));
  DEBUG_SERIAL.flush();
#endif
  //rbSerial.listen();

  if ((rc = isbd.begin()) == ISBD_SUCCESS) {
#ifdef SERIAL_DEBUG_ROCKBLOCK
    DEBUG_SERIAL.flush();
    DEBUG_SERIAL.print(F("Transmitting address="));
    DEBUG_SERIAL.print((long)idPtr);
    DEBUG_SERIAL.print(F(" length="));
    DEBUG_SERIAL.print(idLen);
    DEBUG_SERIAL.print(F("\r\n"));

    DEBUG_SERIAL.flush();
#endif

    recCount = 0;
    wkPtr = (uint8_t*)idPtr;
    chunkPtr = (uint8_t*)&idcChunk.idcBuffer;
    wkLen = idLen;

    while (wkLen > 0) {
      idcChunk.idcSendTime = idPtr->idGPSTime;
      idcChunk.idcRecordType[0] = 'I';
      idcChunk.idcRecordType[1] = 'D';
      idcChunk.idcRecordNumber = recCount;

      if (wkLen < MAX_CHUNK_LENGTH) {
        chunkLen = (MAX_CHUNK_LENGTH + CHUNK_HEADER_SIZE);
        wkLen -= MAX_CHUNK_LENGTH;
        wkPtr += MAX_CHUNK_LENGTH;
      } else {
        chunkLen = (wkLen + CHUNK_HEADER_SIZE);
        wkLen = 0;
      }

      memmove(chunkPtr, idPtr, chunkLen);
      ++recCount;

#ifdef SERIAL_DEBUG_ROCKBLOCK
      DEBUG_SERIAL.flush();
      DEBUG_SERIAL.print(F("Chunk address="));
      DEBUG_SERIAL.print((long)chunkPtr);
      DEBUG_SERIAL.print(F(" Chunk length="));
      DEBUG_SERIAL.print(chunkLen);
      DEBUG_SERIAL.print(F("\r\n"));

      DEBUG_SERIAL.flush();
#endif

#ifdef NEVER_TRANSMIT
  #ifdef SERIAL_DEBUG
      DEBUG_SERIAL.print(F("Transmission disabled by NEVER_TRANSMIT switch.\r\n"));
  #endif
#else
      rc = isbd.sendSBDBinary(chunkPtr, chunkLen);
#endif // NEVER_TRANSMIT

#ifdef SERIAL_DEBUG_ROCKBLOCK
      DEBUG_SERIAL.flush();
      if (rc == 0) {
        DEBUG_SERIAL.print(F("Good return code from send!\r\n"));
        DEBUG_SERIAL.flush();
      } else {
        DEBUG_SERIAL.print(F("Bad return code from send = "));
        DEBUG_SERIAL.print(rc);
        DEBUG_SERIAL.print(F("\r\n"));
        DEBUG_SERIAL.flush();
      }
#endif
    }

  } else {
#ifdef SERIAL_DEBUG_ROCKBLOCK
    DEBUG_SERIAL.print("Bad return code from begin = ");
    DEBUG_SERIAL.print(rc);
    DEBUG_SERIAL.print("\r\n");
    DEBUG_SERIAL.flush();
#endif
  }
  
  isbd.sleep();
  rbSerial.end();
  digitalWrite(ROCKBLOCK_POWER_PIN, LOW);
  // Turn the serial mux off.
  serialMuxSetToOff();
}

#ifdef SERIAL_DEBUG_ROCKBLOCK
void ISBDConsoleCallback(IridiumSBD *device, char c) {
  DEBUG_SERIAL.write(c);
}

void ISBDDiagsCallback(IridiumSBD* device, char c) {
  DEBUG_SERIAL.write(c);
}
#endif


