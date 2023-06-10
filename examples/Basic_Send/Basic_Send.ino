#include <Arduino.h>
#include <XModem.h>

/*
 * You can test this over your USB port using lrzsz: `stty -F /dev/ttyUSB0 4800 && rx -vaX /path/to/save/file > /dev/ttyUSB0 < /dev/ttyUSB0`
 * If you want to try CRC_XMODEM then add a c flag to the rx command (-vacX)
 */
void setup() {
  Serial.begin(4800, SERIAL_8N1);
  myXModem.begin(Serial, XModem::ProtocolType::XMODEM);
}

static const char text[] = "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Sed sollicitudin tempor ex et sagittis. Sed feugiat justo ac dui posuere, in porta odio aliquet. Vestibulum dignissim malesuada lacus eu consectetur. Fusce sed varius nibh. Nulla lacinia ipsum non porttitor auctor. Aenean eget eros pharetra, fringilla ipsum eget, pharetra turpis. Suspendisse vitae leo id orci consectetur faucibus. Praesent elementum ex eget venenatis consequat.\n"
"\n"
"Donec hendrerit porta nisl, eget commodo nisl tincidunt non. Nam eleifend auctor lectus at euismod. Praesent maximus, purus quis molestie pellentesque, dui lectus tincidunt elit, vel pharetra arcu felis luctus lacus. Integer iaculis felis sed fringilla suscipit. Pellentesque dapibus, magna vitae molestie placerat, nulla odio elementum purus, at condimentum est tortor et mi. Duis eu lacus sit amet nisl feugiat consequat. Donec condimentum eu enim vel tempus. Nam dapibus urna quis dolor sodales venenatis.";

void loop() {
  //This simple example continuously tries to send data
  myXModem.send(text, strlen(text));
}
