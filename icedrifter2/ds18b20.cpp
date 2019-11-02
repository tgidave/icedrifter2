#include <OneWire.h>
#include <DallasTemperature.h>

#include "icedrifter2.h"
#include "ds18b20.h"

OneWire oneWire(ONE_WIRE_BUS);

DallasTemperature sensors(& oneWire);

float getRemoteTemp(icedrifterData* idData) { 

// pinMode(DS18B20_POWER_PIN, OUTPUT);
  digitalWrite(DS18B20_POWER_PIN, HIGH);
  delay(1000);

  //Start the Library.
  sensors.begin();

#ifdef SERIAL_DEBUG_DS18B20
  DEBUG_SERIAL.print(F("Requesting DS18B20 temperatures...\r\n"));
#endif

  sensors.requestTemperatures(); // Send the command to get temperature readings
  idData->idRemoteTemp = sensors.getTempCByIndex(0);

#ifdef SERIAL_DEBUG_DS18B20
  DEBUG_SERIAL.print(F("Remote temperature is "));
  DEBUG_SERIAL.print(idData->idRemoteTemp);
  DEBUG_SERIAL.print(F(" C\r\n"));

  if (idData->idRemoteTemp == -127) {
    DEBUG_SERIAL.print(F("Error: Disconnected!!!\r\n"));
  } else {
    DEBUG_SERIAL.print(F("Done!\r\n"));
  }
#endif
  digitalWrite(DS18B20_POWER_PIN, LOW);
}
