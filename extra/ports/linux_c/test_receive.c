#define XMODEM_DEBUG
#define XMODEM_ALLOW_NONSEQUENTIAL
#define XMODEM_BUFFER_PACKET_READS
#include "xmodem.c"
#include <string.h>
#include <stdio.h>
#include <unistd.h>

bool handler(void *blk_id, size_t id_len, char *data, size_t data_len) {
  printf("\nrx:\n");
  write(fileno(stdout), data, data_len);
  return true;
}

int main(int argc, char** argv) {
  printf("opening device\n");
  //struct serial_device *dev = open_serial("/dev/ttyUSB0", B9600);
  struct serial_device *dev = open_serial("/dev/ttyUSB0", B4800);
  if(dev == NULL) {
    printf("Error %i from open_serial: %s\n", errno, strerror(errno));
    exit(1);
  }

  //you can modify the library configuration here
  struct xmodem_config config = {};
  config.rx_block_handler = handler;
  apply_config(dev, &config);

  while(!xmodem_receive(dev)) {}
  printf("\nDone");

  printf("\nclosing device\n");
  close_serial(dev);

  return 0;
}
