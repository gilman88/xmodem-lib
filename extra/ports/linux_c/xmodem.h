#ifndef XMODEM_LIB
#define XMODEM_LIB

#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <termios.h>
#include <stdlib.h>
#include <time.h>
#include <stdbool.h>

enum x_mode {
  XMODEM,
  CRC_XMODEM
};
struct serial_device;

struct xmodem_config {
  size_t id_bytes;
  size_t data_bytes;
  size_t chksm_bytes;
  unsigned char rx_init_byte;
  //function pointer handlers
  bool (*rx_block_handler) (void *blk_id, size_t id_len, char *data, size_t data_len);
  void (*calc_chksum) (unsigned char *data, size_t data_bytes, unsigned char *chksm);
};

struct serial_device* open_serial(char* device_path, int baudrate, int flags, enum x_mode mode);
#define open_serial(path, rate, ...) open_serial_default(path, rate, ##__VA_ARGS__, 0, XMODEM)
#define open_serial_default(path, rate, flags, mode, ...) open_serial(path, rate, flags, mode)

//zero or null value members of the new config are invalid and are used as don't update flags
void apply_config(struct serial_device *dev, struct xmodem_config *config);
void close_serial(struct serial_device *dev);
void print_byte(int fd, unsigned char byte);

bool xmodem_receive(struct serial_device *dev);
bool xmodem_send(struct serial_device *dev, char *data, size_t data_len, unsigned long long start_id);
#define xmodem_send(dev, data, data_len, ...) xmodem_send_default(dev, data, data_len, ##__VA_ARGS__, 1)
#define xmodem_send_default(dev, data, data_len, id, ...) xmodem_send(dev, data, data_len, id)

struct xmodem_bulk_data {
  char **data_arr;
  size_t *len_arr;
  //each id is xmodem_config.id_bytes long in big endian format
  unsigned char **id_arr;
  size_t count;
};

bool xmodem_send_bulk_data(struct serial_device *dev, struct xmodem_bulk_data container);

#endif
