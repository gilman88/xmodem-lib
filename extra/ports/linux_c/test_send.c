#define XMODEM_DEBUG
#define XMODEM_ALLOW_NONSEQUENTIAL
#include "xmodem.c"
#include <string.h>
#include <stdio.h>

int main(int argc, char** argv) {
  printf("opening device\n");
  //struct serial_device *dev = open_serial("/dev/ttyUSB0", B9600);
  struct serial_device *dev = open_serial("/dev/ttyUSB0", B4800);
  if(dev == NULL) {
    printf("Error %i from open_serial: %s\n", errno, strerror(errno));
    exit(1);
  }

  //you can modify the library configuration here
  /*
  struct xmodem_config config = {};
  config.id_bytes = 2;
  config.data_bytes = 64;
  config.chksm_bytes = 1;
  apply_config(dev, &config);
  */

  char *data = "This_is_a_64_byte_xmodem_packet.Normally_they_are_128_bytes_tho.tHIS_IS_A_64_BYTE_XMODEM_PACKET.nORMALLY_THEY_ARE_128_BYTES_THO.This_is_a_64_byte_xmodem_packet.Normally_they_are_128_bytes_tho.";
  while(!xmodem_send(dev, data, strlen(data))) {}
  printf("\nDone");

  printf("\nclosing device\n");
  close_serial(dev);

  return 0;
}
