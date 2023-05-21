#include <XModem.h>
#include <Arduino.h>

bool process_block( byte *data, size_t dataSize) {
  
  for(int i = 0; i < dataSize; ++i) {
    //do stuff with the recieved data
  }

  //return false to stop the transfer
  return true;
}

/*
 * You can test this over your USB port using lrzsz: `stty -F /dev/ttyUSB0 4800 && sx -vaX /path/to/send/file > /dev/ttyUSB0 < /dev/ttyUSB0`
 * If you want to try CRC_XMODEM then add a o flag to the sx command (-vaoX)
 */
void setup() {
  Serial.begin(4800, SERIAL_8N1);
  myXModem.begin(Serial, XModem::ProtocolType::XMODEM);
  myXModem.onXmodemReceive(process_block);
}

void loop() {
  //This simple example continuously tries to read data
  myXModem.receive();
}

