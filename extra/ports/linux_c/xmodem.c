#ifndef XMODEM_LIB_IMP
#define XMODEM_LIB_IMP

#include "xmodem.h"
#include <string.h>

#ifdef XMODEM_DEBUG
#define debug_print(format, ...) fprintf(stdout, format, #__VA_ARGS__)
#define debug_print_byte(byte) print_byte(fileno(stdout), byte)
#else
#define debug_print(...) do { } while(0)
#define debug_print_byte(...) do { } while(0)
#endif

#define RETRY_LIMIT 10
#define SIGNAL_RETRY_DELAY_MICRO_SEC 99999

//opaque structs
struct serial_device {
  int fd;
  struct xmodem_config config;
};

bool setup_serial(int fd, int baudrate, int flags);
void increment_id(unsigned char *id, size_t length);
bool find_byte_timed(struct serial_device *dev, unsigned char byte, int timeout_secs);

//XMODEM constants
#define SOH (unsigned char) 0x01 //Start of Header
#define EOT (unsigned char) 0x04 //End of Transmission
#define ACK (unsigned char) 0x06 //Acknowledge
#define NAK (unsigned char) 0x15 //Negative Acknowledge
#define CAN (unsigned char) 0x18 //Cancel
#define SUB (unsigned char) 0x1A //Padding

struct xmodem_packet {
  unsigned char *id;
  unsigned char *chksm;
  char *data;
};

bool _xmodem_init_rx(struct serial_device *dev);
bool find_header(struct serial_device *dev);
bool _xmodem_rx(struct serial_device *dev);
bool _xmodem_init_tx(struct serial_device *dev);
bool _xmodem_tx(struct serial_device *dev, struct xmodem_packet *p, char *data, size_t data_len, char *blk_id);
bool _xmodem_read_block(struct serial_device *dev, struct xmodem_packet *p, unsigned char *buffer);
bool _xmodem_fill_buffer(struct serial_device *dev, unsigned char *buffer, size_t packet_bytes);
unsigned char _xmodem_tx_signal(struct serial_device *dev, unsigned char signal);
unsigned char _xmodem_rx_signal(struct serial_device *dev);
bool _xmodem_send_packet(struct serial_device *dev, struct xmodem_packet *p);
void _xmodem_build_packet(struct xmodem_config *config, struct xmodem_packet *p, unsigned char *id, char *data, size_t data_len);
void fill_checksum_basic(unsigned char *data, size_t data_bytes, unsigned char *chksm);
void fill_checksum_crc_16(unsigned char *data, size_t data_bytes, unsigned char *chksm);
bool dummy_rx_block_handler(void *blk_id, size_t id_len, char *data, size_t data_len);
bool _xmodem_close_tx(struct serial_device *dev);

struct serial_device* open_serial(char* device_path, int baudrate, int flags, enum x_mode mode) {
  struct serial_device *dev = malloc(sizeof(struct serial_device));
  if(dev == NULL) return NULL;

  switch(mode) {
    case XMODEM:
      dev->config.id_bytes = 1;
      dev->config.data_bytes = 128;
      dev->config.chksm_bytes = 1;
      dev->config.rx_init_byte = NAK;
      dev->config.calc_chksum = fill_checksum_basic;
      break;
    case CRC_XMODEM:
      dev->config.id_bytes = 1;
      dev->config.data_bytes = 128;
      dev->config.chksm_bytes = 2;
      dev->config.rx_init_byte = 'C';
      dev->config.calc_chksum = fill_checksum_crc_16;
      break;
  }

  dev->config.rx_block_handler = dummy_rx_block_handler;

  dev->fd = open(device_path, O_RDWR);
  if(dev->fd < 0) {
    return NULL;
  }
  if(!setup_serial(dev->fd, baudrate, flags)) {
    //error close device
    close_serial(dev);
    return NULL;
  }
  return dev;
}

void apply_config(struct serial_device *dev, struct xmodem_config *config) {
  //only change values that are not 0 or NULL
  if(config->id_bytes) dev->config.id_bytes = config->id_bytes;
  if(config->data_bytes) dev->config.data_bytes = config->data_bytes;
  if(config->chksm_bytes) dev->config.chksm_bytes = config->chksm_bytes;
  if(config->rx_init_byte) dev->config.rx_init_byte = config->rx_init_byte;
  if(config->rx_block_handler != NULL) dev->config.rx_block_handler = config->rx_block_handler;
  if(config->calc_chksum != NULL) dev->config.calc_chksum = config->calc_chksum;
}

void close_serial(struct serial_device *dev) {
  close(dev->fd);
  free(dev);
}

void fill_checksum_basic(unsigned char *data, size_t data_bytes, unsigned char *chksm) {
  unsigned char sum = 0;
  for(size_t i = 0; i < data_bytes; ++i) sum += data[i];
  *chksm = sum;
}

void fill_checksum_crc_16(unsigned char *data, size_t data_bytes, unsigned char *chksm) {
  //XModem CRC prime number is 69665 -> 2^16 + 2^12 + 2^5 + 2^0 -> 10001000000100001 -> 0x11021
  //normal notation of this bit pattern omits the leading bit and represents it as 0x1021
  //in code we can omit the 2^16 term due to shifting before XORing when the MSB is a 1
  const unsigned short crc_prime = 0x1021;
  unsigned short *crc = (unsigned short *) chksm;
  *crc = 0;

  //We can ignore crc calulations that cross byte boundaries by just assuming
  //that the following byte is 0 and then fixup our simplification at the end
  //by XORing in the true value of the next byte into the most sygnificant byte
  //of the CRC
  for(size_t i = 0; i < data_bytes; ++i) {
    *crc ^= (((unsigned short) data[i]) << 8);
    for(unsigned char j = 0; j < 8; ++j) {
      if(*crc & 0x8000) *crc = (*crc << 1) ^ crc_prime;
      else *crc <<= 1;
    }
  }
}

bool dummy_rx_block_handler(void *blk_id, size_t id_len, char *data, size_t data_len) { return true; }

void print_byte(int fd, unsigned char byte) {
  static char *lower_ascii_lookup = "<nul>\0<soh>\0<stx>\0<etx>\0<eot>\0<enq>\0<ack>\0<bel>\0<bs>\0\0"
    "<tab>\0<lf>\0\0<vt>\0\0<ff>\0\0<cr>\0\0<so>\0\0<si>\0\0<dle>\0<dc1>\0<dc2>\0<dc3>\0<dc4>\0<nak>\0"
    "<syn>\0<etb>\0<can>\0<em>\0\0<sub>\0<esc>\0<fs>\0\0<gs>\0\0<rs>\0\0<us>\0\0<space>";
  static char *upper_ascii_lookup = "<del>\0<128>\0<129>\0<130>\0<131>\0<132>\0<133>\0<134>\0<135>\0<136>\0"
    "<137>\0<138>\0<139>\0<140>\0<141>\0<142>\0<143>\0<144>\0<145>\0<146>\0<147>\0<148>\0<149>\0<150>\0<151>\0"
    "<152>\0<153>\0<154>\0<155>\0<156>\0<157>\0<158>\0<159>\0<160>\0<161>\0<162>\0<163>\0<164>\0<165>\0<166>\0"
    "<167>\0<168>\0<169>\0<170>\0<171>\0<172>\0<173>\0<174>\0<175>\0<176>\0<177>\0<178>\0<179>\0<180>\0<181>\0"
    "<182>\0<183>\0<184>\0<185>\0<186>\0<187>\0<188>\0<189>\0<190>\0<191>\0<192>\0<193>\0<194>\0<195>\0<196>\0"
    "<197>\0<198>\0<199>\0<200>\0<201>\0<202>\0<203>\0<204>\0<205>\0<206>\0<207>\0<208>\0<209>\0<210>\0<211>\0"
    "<212>\0<213>\0<214>\0<215>\0<216>\0<217>\0<218>\0<219>\0<220>\0<221>\0<222>\0<223>\0<224>\0<225>\0<226>\0"
    "<227>\0<228>\0<229>\0<230>\0<231>\0<232>\0<233>\0<234>\0<235>\0<236>\0<237>\0<238>\0<239>\0<240>\0<241>\0"
    "<242>\0<243>\0<244>\0<245>\0<246>\0<247>\0<248>\0<249>\0<250>\0<251>\0<252>\0<253>\0<254>\0<255>";

  if(byte < '!') {
    char *c = &lower_ascii_lookup[((unsigned int) byte)*6];
    write(fd, c, strlen(c));
  } else if(byte <= '~') {
    write(fd, &byte, 1);
  } else {
    char *c = &upper_ascii_lookup[((unsigned int) byte-'~'-1)*6];
    write(fd, c, strlen(c));
  }
}

bool setup_serial(int fd, int baudrate, int flags) {
  if(flags == 0) {}//TODO: Set default flags?

  struct termios tty;
  memset((char *)&tty, 0, sizeof(struct termios));

  if(tcgetattr(fd, &tty) != 0) return false;

  //PARENB seems to not be getting set properly.... TODO: Is this valid?
  tty.c_cflag |= ( 0
      | PARENB    //Enable Parity
      | CS8       //8 bits per byte
      | CREAD     //Enable receiver
      | CLOCAL);  //Ignore model control lines

  tty.c_cflag &= ~( 0
      | CSTOPB    //single stop bit
      | CRTSCTS); //disable hardware flow control

  tty.c_lflag &= ~( 0
      | ICANON   //non-canonical input mode
      | ECHO     //disable echo
      | ECHOE    //disable erasure
      | ECHONL   //disable new-line echo
      | ISIG);   //disable INTR, QUIT, SUSP control characters

  tty.c_iflag &= ~( 0
      | IGNBRK|BRKINT|PARMRK|ISTRIP|INLCR|IGNCR|ICRNL);  //disable special byte sequences

  tty.c_oflag &= ~( 0
      | OPOST    //disable special interpretations of output eg newlines
      | ONLCR);  //disable conversion of newline to CRLF

  tty.c_cc[VTIME] = 30; //reads wait for up to 3s
  tty.c_cc[VMIN] = 0; //reads return as soon as data is available
  //set baud rate
  cfsetispeed(&tty, baudrate);
  cfsetospeed(&tty, baudrate);

  //commit tty settings
  if(tcsetattr(fd, TCSANOW, &tty) != 0) return false;
  return true;
}

bool xmodem_receive(struct serial_device *dev) {
  if(!_xmodem_init_rx(dev) || !_xmodem_rx(dev)) {
    //an unrecoverable error occured send cancels to terminate the transcation
    debug_print("\nError Cancelling transfer");
    unsigned char b = CAN;
    write(dev->fd, &b, 1);
    write(dev->fd, &b, 1);
    write(dev->fd, &b, 1);
    return false;
  }
  return true;
}

bool xmodem_send(struct serial_device *dev, char *data, size_t data_len, unsigned long long start_id) {
  unsigned char *id = malloc(dev->config.id_bytes);

  //convert the start id to big endian format
  unsigned long long temp = start_id;
  for(size_t i = 0; i < dev->config.id_bytes; ++i) {
    id[dev->config.id_bytes-i-1] = (unsigned char) (temp & 0xFF);
    temp >>= 8;
  }

  struct xmodem_bulk_data container;
  container.data_arr = &data;
  container.len_arr = &data_len;
  container.id_arr = &id;
  container.count = 1;

  bool result = xmodem_send_bulk_data(dev, container);
  free(id);
  return result;
}

bool xmodem_send_bulk_data(struct serial_device *dev, struct xmodem_bulk_data container) {
  if(container.count == 0) return false;

  struct xmodem_packet p;

  //bundle all our memory allocations together
  //need to store:
  //2 id blocks - blk_id and xmodem_packet struct
  //1 checksum block - xmodem_packet struct
  //1 data block - xmodem_packet struct
  unsigned char *buffer = malloc(2*dev->config.id_bytes + dev->config.chksm_bytes + dev->config.data_bytes);
  unsigned char *blk_id = buffer + dev->config.data_bytes;
  p.id = blk_id + dev->config.id_bytes;
  p.chksm = p.id + dev->config.id_bytes;
  p.data = buffer;

  bool result = _xmodem_init_tx(dev);
  while(result && container.count-- != 0) {
    for(size_t i = 0; i < dev->config.data_bytes; ++i) blk_id[i] = container.id_arr[container.count][i];
    result &= _xmodem_tx(dev, &p, container.data_arr[container.count], container.len_arr[container.count], blk_id);
  }

  if(result) {
    debug_print("\nClosing xmodem transfer:");
    _xmodem_close_tx(dev);
  } else {
    //an unrecoverable error occured send cancels to terminate the transcation
    debug_print("\nError Cancelling transfer");
    unsigned char b = CAN;
    write(dev->fd, &b, 1);
    write(dev->fd, &b, 1);
    write(dev->fd, &b, 1);
  }

  debug_print("\nDone");
  free(buffer);
  return result;
}

inline void increment_id(unsigned char *id, size_t length) {
  size_t index = length-1;
  do {
    id[index]++;
    if(id[index]) return; //if the current byte is non-zero then there is no overflow and we are done
  } while(index--);//when we hit an index of zero then we have incremented all the bytes
}

bool find_byte_timed(struct serial_device *dev, unsigned char byte, int timeout_secs) {
  unsigned char b;
  time_t end = time(NULL) + timeout_secs;
  do {
    //if no data is available sleep and check again before checking the value of b
    if(!read(dev->fd, &b, 1)) {
      usleep(500);
      read(dev->fd, &b, 1);
    }

    if(b == byte) return true;
  } while(time(NULL) < end);
  return false;
}

bool _xmodem_init_rx(struct serial_device *dev) {
  debug_print("Initializing Receive Transaction... ");
  unsigned char i = 0;
  do {
    write(dev->fd, &dev->config.rx_init_byte, 1);
    if(find_byte_timed(dev, SOH, 10)) {
      debug_print("Done\n");
      return true;
    }
  } while(i++ < RETRY_LIMIT);
  return false;
}

bool find_header(struct serial_device *dev) {
  unsigned char i = 0;
  static unsigned char retry_byte = NAK;
  do {
    if(i != 0) write(dev->fd, &retry_byte, 1);
    if(find_byte_timed(dev, SOH, 10)) return true;
  } while(i++ < RETRY_LIMIT);
  return false;
}

bool _xmodem_rx(struct serial_device *dev) {
  bool result = false;

  unsigned char *buffer;
  unsigned char *prev_blk_id;
  unsigned char *expected_id;
  struct xmodem_packet p;

  //bundle all our memory allocations together
#if defined(XMODEM_BUFFER_PACKET_READS)
  //need to store:
  //5 id blocks - prev_blk_id, expected_id, xmodem_packet struct, buffer id and buffer compl_id
  //2 chksum block - xmodem_packet struct and buffer chksum
  //2 data block - xmodem_packet struct and buffer data
  buffer = malloc(5*dev->config.id_bytes + 2*dev->config.chksm_bytes + 2*dev->config.data_bytes);
  prev_blk_id = buffer + 2*dev->config.id_bytes + dev->config.chksm_bytes + dev->config.data_bytes;
#else
  //need to store:
  //3 id blocks - prev_blk_id, expected_id and xmodem_packet struct
  //1 checksum block - xmodem_packet struct
  //1 data block - xmodem_packet struct
  buffer = malloc(3*dev->config.id_bytes + dev->config.chksm_bytes + dev->config.data_bytes);
  prev_blk_id = buffer;
#endif

  expected_id = prev_blk_id + dev->config.id_bytes;
  p.id = expected_id + dev->config.id_bytes;
  p.chksm = p.id + dev->config.id_bytes;
  p.data = p.chksm + dev->config.chksm_bytes;

  for(size_t i = 0; i < dev->config.id_bytes; ++i) prev_blk_id[i] = expected_id[i] = 0;

  unsigned char errors = 0;
  while(true) {
    if(_xmodem_read_block(dev, &p, buffer)) {
      //reset errors
      errors = 0;

      //ignore resends of the last received block
      size_t matches = 0;
      for(size_t i = 0; i < dev->config.id_bytes; ++i) {
        if(prev_blk_id[i] == p.id[i]) ++matches;
      }

      //if its a duplicate block we still need to send an ACK
      if(matches != dev->config.id_bytes) {

#if defined(XMODEM_ALLOW_NONSEQUENTIAL)
        for(size_t i = 0; i < dev->config.id_bytes; ++i) expected_id[i] = p.id[i];
#else
        increment_id(expected_id, dev->config.id_bytes);

        matches = 0;
        for(size_t i = 0; i < dev->config.id_bytes; ++i) {
          if(expected_id[i] == p.id[i]) ++matches;
        }

        if(matches != dev->config.id_bytes) break;
#endif

        size_t padding_bytes = 0;
        //count number of padding SUB bytes
        while(p.data[dev->config.data_bytes - 1 - padding_bytes] == SUB) ++padding_bytes;

        //process packet
        if(!dev->config.rx_block_handler(p.id, dev->config.id_bytes, p.data, dev->config.data_bytes - padding_bytes)) break;

        for(size_t i = 0; i < dev->config.id_bytes; ++i) prev_blk_id[i] = expected_id[i];
      }

      //signal acknowledgement
      unsigned char response = _xmodem_tx_signal(dev, ACK);
      if(response == CAN) break;
      if(response == EOT) {
        response = _xmodem_tx_signal(dev, NAK);
        if(response == CAN) break;
        if(response == EOT) {
          buffer[0] = ACK;
          write(dev->fd, buffer, 1);
          result = true;
          break;
        }
      }
      //Unexpected response and resync attempt failed so fail out
      if(response != SOH && !find_header(dev)) break;
    } else {
      if(++errors > RETRY_LIMIT) break;
      unsigned char response = _xmodem_tx_signal(dev, NAK);
      if(response == CAN) break;
      if(response != SOH && !find_header(dev)) break;
    }
  }

  free(buffer);
  return result;
}

bool _xmodem_read_block(struct serial_device *dev, struct xmodem_packet *p, unsigned char *buffer) {
  debug_print("\nReading packet ");
#if defined(XMODEM_BUFFER_PACKET_READS)
  size_t b_pos = 2*dev->config.id_bytes + dev->config.chksm_bytes + dev->config.data_bytes;
  if(!_xmodem_fill_buffer(dev, buffer, b_pos)) return false;

  b_pos = 0;
  for(size_t i = 0; i < dev->config.id_bytes; ++i) {
    p->id[i] = buffer[b_pos++];
    debug_print_byte(p->id[i]);
    debug_print_byte(buffer[b_pos]);

    /*
       possibly because of C integer promotion rules the ~ operator changes
       the variable type of an unsigned char to a char so we need to cast
       it back. I think it goes something like this:
       a -> 0x00 - 0xff (unsigned char)
       ~a -> 0xffffffff - 0x00000000 (integer promotion to int for ~ operation)
       all values are now negative so its no longer possible to promote
       back to an unsigned char
       */

    if(p->id[i] != (unsigned char) ~buffer[b_pos++]) return false;
  }
  debug_print(": ");

  for(size_t i = 0; i < dev->config.data_bytes; ++i) {
    p->data[i] = buffer[b_pos++];
    debug_print_byte(p->data[i]);
  }

  for(size_t i = 0; i < dev->config.chksm_bytes; ++i) {
    debug_print_byte(buffer[b_pos + i]);
  }

  dev->config.calc_chksum(p->data, dev->config.data_bytes, p->chksm);
  for(size_t i = 0; i < dev->config.chksm_bytes; ++i) {
    if(p->chksm[i] != buffer[b_pos++]) return false;
  }
#else
  //can only read 1 byte at a time because of our serial device settings VMIN/VTIME
  unsigned char tmp;
  for(size_t i = 0; i < dev->config.id_bytes; ++i) {
    if(read(dev->fd, p->id + i, 1) <= 0) return false;
    if(read(dev->fd, &tmp, 1) <= 0) return false;

    debug_print_byte(p->id[i]);
    debug_print_byte(tmp);

    /*
       possibly because of C integer promotion rules the ~ operator changes
       the variable type of an unsigned char to a char so we need to cast
       it back. I think it goes something like this:
       a -> 0x00 - 0xff (unsigned char)
       ~a -> 0xffffffff - 0x00000000 (integer promotion to int for ~ operation)
       all values are now negative so its no longer possible to promote
       back to an unsigned char
       */

    if(p->id[i] != (unsigned char) ~tmp) return false;
  }
  debug_print(": ");

  if(!_xmodem_fill_buffer(dev, p->data, dev->config.data_bytes)) return false;
  for(size_t i = 0; i < dev->config.data_bytes; ++i) debug_print_byte(p->data[i]);

  dev->config.calc_chksum(p->data, dev->config.data_bytes, p->chksm);
  for(size_t i = 0; i < dev->config.chksm_bytes; ++i) {
    if(read(dev->fd, &tmp, 1) <= 0) return false;
    debug_print_byte(tmp);
    if(p->chksm[i] != tmp) return false;
  }
#endif

  return true;
}

bool _xmodem_fill_buffer(struct serial_device *dev, unsigned char *buffer, size_t bytes) {
  size_t count = 0;
  while(count < bytes) {
    ssize_t r = read(dev->fd, buffer + count, bytes - count);

    //the baud rate / sending device may be much slower than ourselves so
    //we only signal an error condition if no data has been received at all
    //within the tty timeout period (VTIME)
    if(r <= 0) return false;

    count += r;
  }
  return true;
}

bool _xmodem_init_tx(struct serial_device *dev) {
  debug_print("Initializing Send Transaction... ");
  unsigned char i = 0;
  do {
    if(find_byte_timed(dev, dev->config.rx_init_byte, 60)) {
      debug_print("Done\n");
      return true;
    }
  } while(i++ < RETRY_LIMIT);
  return false;
}

bool _xmodem_tx(struct serial_device *dev, struct xmodem_packet *p, char *data, size_t data_len, char *blk_id) {
  char *data_ptr = data;
  char *data_end = data_ptr + data_len;

  //flush the incoming stream before starting
  tcflush(dev->fd, TCIFLUSH);

  while(data_ptr + dev->config.data_bytes < data_end) {
    _xmodem_build_packet(&dev->config, p, blk_id, data_ptr, dev->config.data_bytes);
    increment_id(blk_id, dev->config.id_bytes);
    if(!_xmodem_send_packet(dev, p)) return false;
    data_ptr += dev->config.data_bytes;
  }

  if(data_ptr != data_end) {
    memset(p->data, SUB, dev->config.data_bytes); //set all bytes to the padding byte

    _xmodem_build_packet(&dev->config, p, blk_id, data_ptr, data_end - data_ptr);
    if(!_xmodem_send_packet(dev, p)) return false;
  }

  return true;
}

bool _xmodem_close_tx(struct serial_device *dev) {
  unsigned char error_responses = 0;
  while(error_responses < RETRY_LIMIT) {
    unsigned char response = _xmodem_tx_signal(dev, EOT);
    if(response == ACK) return true;
    if(response == NAK) continue;
    if(response == CAN) {
      if(_xmodem_rx_signal(dev) == CAN) break;
    } else ++error_responses;
  }
  return false;
}

void _xmodem_build_packet(struct xmodem_config *config, struct xmodem_packet *p, unsigned char *id, char *data, size_t data_len) {
  debug_print("\nBuilding packet for block ");
  for(size_t i = 0; i < config->id_bytes; ++i) debug_print_byte(id[i]);
  debug_print("\n");

  memcpy(p->id, id, config->id_bytes);
  memcpy(p->data, data, data_len);
  config->calc_chksum(p->data, config->data_bytes, p->chksm);
}

bool _xmodem_send_packet(struct serial_device *dev, struct xmodem_packet *p) {
  static unsigned char start_byte = SOH;

  debug_print("Packet:\n");
  debug_print_byte(start_byte);
  for(size_t i = 0; i < dev->config.id_bytes; ++i) {
    debug_print_byte(p->id[i]);
    debug_print_byte(~p->id[i]);
  }
  for(size_t i = 0; i < dev->config.data_bytes; ++i) debug_print_byte(p->data[i]);
  for(size_t i = 0; i < dev->config.chksm_bytes; ++i) debug_print_byte(p->chksm[i]);

  char tries = 0;
  do {
    debug_print("\nSending packet: ");
    --tries;
    //Sending packet
    write(dev->fd, &start_byte, 1);
    for(size_t i = 0; i < dev->config.id_bytes; ++i) {
      unsigned char compl = ~p->id[i];
      write(dev->fd, p->id+i, 1);
      write(dev->fd, &compl, 1);
    }
    write(dev->fd, p->data, dev->config.data_bytes);
    write(dev->fd, p->chksm, dev->config.chksm_bytes);
    debug_print("Done ");

    //Waiting for response
    unsigned char response = _xmodem_rx_signal(dev);
    if(response == ACK) return true;
    if(response == NAK) continue;
    if(response == CAN) {
      if(_xmodem_rx_signal(dev) == CAN) break;
    }
  } while(tries < RETRY_LIMIT);

  return false;
}

unsigned char _xmodem_tx_signal(struct serial_device *dev, unsigned char signal) {
  if(signal == NAK) {
    //make sure the line is clear
    //TODO: better approach?
    sleep(1);
    tcflush(dev->fd, TCIFLUSH);
  }

  debug_print_byte(signal);
  debug_print("->");
  unsigned char i = 0;
  unsigned char b;
  do {
    unsigned char x = 0;
    write(dev->fd, &signal, 1);
    while(read(dev->fd, &b, 1) != 1 && ++x < RETRY_LIMIT) usleep(SIGNAL_RETRY_DELAY_MICRO_SEC);

    debug_print_byte(b);
    switch(b) {
      case SOH:
      case EOT:
      case CAN:
      case ACK:
      case NAK:
        return b;
    }
  } while(++i < RETRY_LIMIT);
  return 255;
}

unsigned char _xmodem_rx_signal(struct serial_device *dev) {
  unsigned char i = 0;
  unsigned char b;
  while(read(dev->fd, &b, 1) != 1 && ++i < RETRY_LIMIT) usleep(SIGNAL_RETRY_DELAY_MICRO_SEC);

  debug_print_byte(b);
  switch(b) {
    case ACK:
    case NAK:
    case CAN:
      return b;
  }
  return 255;
}

#undef SOH
#undef EOT
#undef ACK
#undef NAK
#undef CAN
#undef SUB
#undef debug_print
#undef debug_print_byte
#endif
