#define microSD_CS_PIN 17
#include <SPI.h>
#include <SD.h>
#include <Arduino.h>
#include <XModem.h>

/*
 * Serial1 is XModem 9600
 * Serial2 is Console prompt 57600
 * 
 * You can test this over your USB port using lrzsz: `stty -F /dev/ttyS21 9600 && rx -vX /path/to/save/flile > /dev/ttyS21 < /dev/ttyS21`
 * If you want to try CRC_XMODEM then add a c flag to the rx command (-vacX)
 */


String fileNameBuffer = "opena.sh";

File root;
void printDirectory(File dir, int numTabs) {
  while (true) {

    File entry =  dir.openNextFile();
    if (! entry) {
      // no more files
      break;
    }
    for (uint8_t i = 0; i < numTabs; i++) {
      Serial2.print('\t');
    }
    Serial2.print(entry.name());
    if (entry.isDirectory()) {
      Serial2.println("/");
      printDirectory(entry, numTabs + 1);
    } else {
      // files have sizes, directories do not
      Serial2.print("\t\t");
      Serial2.print(entry.size(), DEC);
      time_t cr = entry.getCreationTime();
      time_t lw = entry.getLastWrite();
      struct tm * tmstruct = localtime(&cr);
      Serial2.printf("\tCREATION: %d-%02d-%02d %02d:%02d:%02d", (tmstruct->tm_year) + 1900, (tmstruct->tm_mon) + 1, tmstruct->tm_mday, tmstruct->tm_hour, tmstruct->tm_min, tmstruct->tm_sec);
      tmstruct = localtime(&lw);
      Serial2.printf("\tLAST WRITE: %d-%02d-%02d %02d:%02d:%02d\r\n", (tmstruct->tm_year) + 1900, (tmstruct->tm_mon) + 1, tmstruct->tm_mday, tmstruct->tm_hour, tmstruct->tm_min, tmstruct->tm_sec);
    }
    entry.close();
  }
}
void printSd(){
// Open serial communications and wait for port to open:
  //PIPManager::pathAssert("user12/key13");
  Serial2.print("Initializing SD card...");

  // Ensure the SPI pinout the SD card is connected to is configured properly
  if (!SD.begin(microSD_CS_PIN)) {
    Serial2.println("initialization failed!");
    return;
  }
  Serial2.println("initialization done.");

  root = SD.open("/");

  printDirectory(root, 0);

  Serial2.println("done!");
  SD.end(false);
}
void resetFunc (){
  Serial2.println("reset");
  delay(50000);
watchdog_enable(1, 1);
    while(1);
}

void setup() {
  SPI.begin();
  Serial1.begin(9600);
  Serial2.begin(57600);
  Serial2.setTimeout(30000);
  Serial2.println("boot Serial 2");

  printSd();
  Serial2.println("File name?");
  Serial2.flush();

  fileNameBuffer = Serial2.readStringUntil('\n');
  if(fileNameBuffer.length() ==0)resetFunc();
  
  Serial2.println("read from: "+fileNameBuffer);
  Serial2.flush();
  myXModem.begin(Serial1, XModem::ProtocolType::XMODEM);
  Serial2.println("lets go!");
}

void loop() {
  //This simple example continuously tries to send data
  myXModem.sendFile(fileNameBuffer);
  resetFunc();
}
