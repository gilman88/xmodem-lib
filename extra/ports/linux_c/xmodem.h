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

struct xmodem_config {
  size_t id_bytes;
  size_t data_bytes;
  size_t chksm_bytes;
  unsigned char rx_init_byte;
  //function pointer handlers
  bool (*rx_block_handler) (void *blk_id, size_t id_len, unsigned char *data, size_t data_len);
  void (*block_lookup) (void *blk_id, size_t id_len, unsigned char *send_data, size_t data_len);
  void (*calc_chksum) (unsigned char *data, size_t data_bytes, unsigned char *chksm);
};

void init_config(struct xmodem_config* config, enum x_mode mode);
#define init_config(config, ...) init_config_default(config, ##__VA_ARGS__, XMODEM)
#define init_config_default(config, mode, ...) init_config(config, mode)

void print_byte(int fd, unsigned char byte);

bool xmodem_receive(int *fd, struct xmodem_config *config);
bool xmodem_send(int *fd, struct xmodem_config *config, unsigned char *data, size_t data_len, unsigned long long start_id);
#define xmodem_send(fd, config, data, data_len, ...) xmodem_send_default(fd, config, data, data_len, ##__VA_ARGS__, 1)
#define xmodem_send_default(fd, config, data, data_len, id, ...) xmodem_send(fd, config, data, data_len, id)
bool xmodem_lookup_send(int *fd, struct xmodem_config *config, unsigned long long id);

struct xmodem_bulk_data {
  unsigned char **data_arr;
  size_t *len_arr;
  //each id is xmodem_config.id_bytes long in big endian format
  unsigned char *id_arr;
  size_t count;
};

bool xmodem_send_bulk_data(int *fd, struct xmodem_config *config, struct xmodem_bulk_data container);

#endif
