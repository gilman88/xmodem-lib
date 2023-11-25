#include <XModem.h>
XModem xmodem;
//The arduino toolchain will add these declarations automatically but doing
//manually so things also just work if someone uses a different/custom toolchain
bool process_block(void *blk_id, size_t idSize, byte *data, size_t dataSize);

/*
 * You can test this over your USB port using lrzsz: `stty -F /dev/ttyUSB0 4800 && sx -vaX /path/to/send/file > /dev/ttyUSB0 < /dev/ttyUSB0`
 * If you want to try CRC_XMODEM then add a o flag to the sx command (-vaoX)
 */
void setup() {
  Serial.begin(4800, SERIAL_8N1);
  xmodem.begin(Serial, XModem::ProtocolType::XMODEM);
  xmodem.setRecieveBlockHandler(process_block);
}

void loop() {
  //This simple example continuously tries to read data
  xmodem.receive();
}

bool process_block(void *blk_id, size_t idSize, byte *data, size_t dataSize) {
  byte id = *((byte *) blk_id);
  for(int i = 0; i < dataSize; ++i) {
    //do stuff with the recieved data
  }

  //return false to stop the transfer early
  return true;
}
