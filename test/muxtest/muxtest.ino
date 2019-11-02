/*
 Muxtest

*/
enum muxCmd {
  muxOff,
  muxGPS,
  muxRockBlock,
  muxChain,
  muxCmdMax,
};

enum muxIx {
  muxINH,
  muxA,
  muxB,
};

const uint8_t muxTable[4][3] = {
  { HIGH, LOW, LOW },
  { LOW, LOW, HIGH },
  { LOW, HIGH, LOW },
  { LOW, HIGH, HIGH },
};

#define RXD1        10
#define TXD1        11

#define muxINHPort  19
#define muxAPort    13
#define muxBPort    12

void SetSerialMuxOff(void);
void SetSerialMuxToGPS(void);
void SetSerialMuxToRockBlock(void);
void SetSerialMuxToChain(void); 

// the setup function runs once when you press reset or power the board
void setup() {
  // initialize digital pin LED_BUILTIN as an output.

  Serial.begin(115200);
  Serial.print(F("Mux test...\n"));

  pinMode(muxINHPort, OUTPUT);
  pinMode(muxAPort, OUTPUT);
  pinMode(muxBPort, OUTPUT);
  SetSerialMuxOff();
}

// the loop function runs over and over again forever
void loop() {
  digitalWrite(RXD1, HIGH); 
  digitalWrite(TXD1, HIGH);

  SetSerialMuxToGPS();

  while (0) {
    delay(1000);
  }
}

void SetSerialMux(int muxCommand) {
  if (!((muxCommand >= 0) && (muxCommand < muxCmdMax))) {
    Serial.print(F("Mux command out of range = "));
    Serial.print(muxCommand);
    Serial.print(F("\n"));
    return;
  }

  digitalWrite(muxINHPort, HIGH);
  digitalWrite(muxAPort, muxTable[muxCommand][muxA]);
  digitalWrite(muxBPort, muxTable[muxCommand][muxB]);
  digitalWrite(muxINHPort, muxTable[muxCommand][muxINH]);

}

void SetSerialMuxOff(void) {
  SetSerialMux(muxOff);
}

void SetSerialMuxToGPS(void) {
  SetSerialMux(muxGPS);
}

void SetSerialMuxToRockblock(void) {
  SetSerialMux(muxRockBlock);
}

void SetSerialMuxToChain(void) {
  SetSerialMux(muxChain);
}


