#define microSD_CS_PIN 10
#include <SPI.h>
#include <SD.h>
#include <avr/wdt.h>
#include <XModem.h>
#include <SoftwareSerial.h>

const byte rxPin = 2;
const byte txPin = 3;

// Set up a new SoftwareSerial object
SoftwareSerial mySerial (rxPin, txPin);
/**
 * Being tested on AtMega328P 16Mhz, speed problem detected
 * XModem receive example
 * Serial is XModem 1200
 * mySerial is Console prompt 57600
 * 
 * You can test this over your USB port using lrzsz: `stty -F /dev/ttyUSB0 1200 && sx -vaX /path/to/send/file > /dev/ttyUSB0 < /dev/ttyUSB0`
 * If you want to try CRC_XMODEM then add a o flag to the sx command (-vaoX)
 */

String fileNameBuffer;

File root;

unsigned int fileSize;

void printDirectory(File dir, int numTabs) {
  while (true) {

    File entry =  dir.openNextFile();
    if (! entry) {
      // no more files
      break;
    }
    for (uint8_t i = 0; i < numTabs; i++) {
      mySerial.print('\t');
    }
    mySerial.print(entry.name());
    if (entry.isDirectory()) {
      mySerial.println("/");
      printDirectory(entry, numTabs + 1);
    } else {
      // files have sizes, directories do not
      mySerial.print("\t\t");
      mySerial.print(entry.size(), DEC);
      //time_t cr = entry.getCreationTime();
      //time_t lw = entry.getLastWrite();
      //struct tm * tmstruct = localtime(&cr);
      //mySerial.printf("\tCREATION: %d-%02d-%02d %02d:%02d:%02d", (tmstruct->tm_year) + 1900, (tmstruct->tm_mon) + 1, tmstruct->tm_mday, tmstruct->tm_hour, tmstruct->tm_min, tmstruct->tm_sec);
      //tmstruct = localtime(&lw);
      //mySerial.printf("\tLAST WRITE: %d-%02d-%02d %02d:%02d:%02d\r\n", (tmstruct->tm_year) + 1900, (tmstruct->tm_mon) + 1, tmstruct->tm_mday, tmstruct->tm_hour, tmstruct->tm_min, tmstruct->tm_sec);
    }
    entry.close();
  }
}
void printSd(){
  root = SD.open("/");

  printDirectory(root, 0);
}
bool getMessageFromXmodem(uint8_t code,uint8_t val){
  mySerial.println(code);
  mySerial.println(val);
  mySerial.println('-');
  return true;
}
void resetFunc(){
  SD.end();
  mySerial.println("reset s00n, may eject sd card");
  cli();
  MCUSR &= ~(1<<WDRF);
  wdt_reset();
  WDTCSR |= (1<<WDCE) | (1<<WDE);
  WDTCSR = (~(1<<WDP1) & ~(1<<WDP2)) | ((1<<WDE) | (1<<WDIE) | (1<<WDP3) | (1<<WDP0));
  sei();
  while(1);
}
void setup() {
  
  
  SPI.begin();
  Serial.begin(1200);
  mySerial.begin(57600);
  mySerial.setTimeout(30000);
  mySerial.println("boot Serial 2");

  mySerial.print("Initializing SD card...");
  if (!SD.begin(microSD_CS_PIN)) {
    mySerial.println("initialization failed!");
    return;
  }
  printSd();
  mySerial.println("File name?");
  mySerial.flush();
  fileNameBuffer = mySerial.readStringUntil('\n');
  if(fileNameBuffer.length() ==0)resetFunc();
  myXModem.onXmodemUpdate(&getMessageFromXmodem);
  myXModem.begin(Serial, XModem::ProtocolType::XMODEM);

  mySerial.println("Receiving to: "+fileNameBuffer);
  mySerial.println("File size?");
  mySerial.flush();
  fileSize = (unsigned int)mySerial.parseInt();
  mySerial.println("Receiving size: "+(String)fileSize+"?");
  mySerial.read();
  mySerial.println("lets go!");
}
void loop() {
  mySerial.println(myXModem.receiveFile(fileNameBuffer,fileSize,true));
  resetFunc();
}