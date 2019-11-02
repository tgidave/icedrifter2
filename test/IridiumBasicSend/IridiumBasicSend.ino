#include <IridiumSBD.h>
#include <SoftwareSerial.h>
#include <avr/wdt.h>
#include "serialmux.h"

#define CONSOLE_BAUD 115200

//SoftwareSerial nss(12, 11);
IridiumSBD isbd(Serial1, 44);

static const int ROCKBLOCK_POWER = 15;

void setup() {

  pinMode(ROCKBLOCK_POWER, OUTPUT);
  digitalWrite(ROCKBLOCK_POWER, HIGH);
  delay(1000);

  serialMuxInit();
  serialMuxSetToRockBlock();

  Serial.begin(CONSOLE_BAUD);
  Serial1.begin(9600);
  
  Serial.print("Starting rockblock test\r\n"); 
  delay(1000);
}

void loop() {

  int signalQuality = -1;

  isbd.attachConsole(Serial);
  isbd.setPowerProfile(1);
  isbd.begin();

  int err = isbd.getSignalQuality(signalQuality);
  if (err != 0) {
    Serial.print("SignalQuality failed: error ");
    Serial.println(err);
    return;
  }

  Serial.print("Signal quality is ");
  Serial.println(signalQuality);

  err = isbd.sendSBDText("Hello, world!");
  if (err != 0) {
    Serial.print("sendSBDText failed: error ");
    Serial.println(err);
    return;
  }

  Serial.println("Hey, it worked!");
  Serial.print("Messages left: ");
  Serial.println(isbd.getWaitingMessageCount());
  Serial.print("Test done.\n");

  digitalWrite(ROCKBLOCK_POWER, LOW);
  setSerialMuxOff(); 

  while (1) {
    delay(1000);
  }

}

