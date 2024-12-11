This is a configurable XModem Transfer library for arduino. By default it will
use the official values for the XModem protocol so will be able to communicate
with other XModem devices/software.

The XModem protocol provides a way to reliably transmit raw binary data over a
half-duplex serial connnection, however in most cases it requires at least
enough memory to hold a full data packet in order to work properly.

I created this library in order to send/receive large amounts of data without
significantly reducing the baud rate (KiloBytes worth). This was acheived by
making the XModem protocol constants configurable. However bear in mind that
both devices need to agree on these for the protocol to work.

The two primary methods are send() and receive(); both require dynamic (heap)
memory.  How much they need depends on the size of the XModem packet which has
the following structure:                <ID_BYTES><DATA_BYTES><CHECKSUM_BYTES>

send() will use:                        2*IDSize + 1*ChecksumSize + 1*DataSize
receive() with buffering will use:      5*IDSize + 2*ChecksumSize + 2*DataSize
receive() without buffering will use:   3*IDSize + 1*ChecksumSize + 1*DataSize

The following parameters can be configured:
__________________________________________
|           NAME           |   DEFAULT   |
|----------------------------------------|
|ID size (bytes)           |            1|
|Checksum size (bytes)     |            1|
|Data size (bytes)         |          128|
|Send Initialization Byte  | <NAK> (0x15)|
|Retry Limit               |           10|
|Retry Delay (ms)          |          100|
|Allow NonSequential Blocks|        false|
|Buffer Packet Reads       |         true|
------------------------------------------

There are also setter methods for providing handler functions:

Recieve Block Handler - This handler will be called to process the received
                        data as each packet is recieved.
Block Lookup Handler  - This handler will be called to load data for sending
                        from external data storage based on the block id.
Checksum Handler      - This handler is used to calulate the expected packet
                        checksum both when sending and recieving.

GETTING STARTED

To start using the library all that is needed is to pass a Serial object into
the begin() method as all settings (including handlers) have defaults, however
the default receive handler just discards all data so you should probably also
provide a RecieveBlockHandler and pass it into setRecieveBlockHandler().

The begin() method optionally accepts a ProtocolType enum for initializing
using values of the different XModem protocol variants, the current
ProtocolTypes are:

- XModem::ProtocolType::XMODEM
- XModem::ProtocolType::CRC_XMODEM

There are other XModem variants that we should be able to support but most of
them make the XModem packets much bigger so I have not investigated them.

The default ProtocolType is XModem::ProtocolType::XMODEM which needs the
following amounts of dynamic memory:

send() will use:                        131 bytes (2*1 + 1*1 + 1*128)
receive() with buffering will use:      263 bytes (5*1 + 2*1 + 2*128)
receive() without buffering will use:   132 bytes (3*1 + 1*1 + 1*128)

You can find basic example sketches with comments in the examples folder

PUBILC METHODS

begin(HardwareSerial)
 Initialize the XModem library with the provided serial device and the
 standard XModem constants

begin(HardwareSerial, XModem::ProtocolType)
 Initialize the XModem libaray with the provided serial device and XModem
 constants for the specified ProtocolType

bool receive()
 Start waiting for incoming data. Returns TRUE when the sending device signals
 that the transfer is complete and FALSE if an error occured. Any data received
 will be passed to the Receive Block Handler along with the packet id for
 processing before signalling for the next block, aside from regular protocol
 errors transfers can also be aborted by returning FALSE from the Receive Block
 Handler. If consecutive blocks are recieved with the same block Id then only
 the first instance will be passed to the Recieve Block Handler for processing.

bool send(char[] data, size_t data_len)
 Start attempting to send data. Returns TRUE when the transfer has completed
 succesfully and FALSE if an error occured. The value 1 will be used as the
 packet ID for the first XModem packet.

bool send(char[] data, size_t data_len, unsigned long long start_id)
 Start attempting to send data. Returns TRUE when the transfer has completed
 succesfully and FALSE if an error occured. The provided start_id is truncated
 down to the ID Size least significant bytes and used as the packet ID for the
 first XModem packet. Note that while using a start_id of 0 is possible the
 receiving device will by defualt discard it.

bool lookup_send(unsigned long long start_id)
 Start attempting to send data returned by the Block Lookup Handler. Returns
 TRUE when the transfer has completed successfully and FALSE if an error
 occured. The provided id is truncated down to the ID Size least significant
 bytes and used as the packet ID for the XModem packet, it is also passed to
 the Block Lookup Handler. Note that while using a start_id of 0 is possible
 the receiving device will by defualt discard it.

void setIdSize(size_t)
 Set the number of ID bytes in an XModem packet

void setChecksumSize(size_t)
 Set the number of Checksum bytes in an XModem packet

void setDataSize(size_t)
 Set the number of Data bytes in an Xmodem packet

void setSendInitByte(byte)
 Set the byte that will be used to initiate XModem transfers

void setRetryLimit(byte)
 Set the maximum number of times to retry to recover from communication errors

void setSignalRetryDelay(unsigned long)
 Set the number of ms after sending a synchronization signal before resending

void allowNonSequentailBlocks(bool)
 XModem transfers officially start with a packet id of 1 and each subsequent
 packet increments due to this receiving non-sequential packet ids are treated
 as a unrecoverable error. Setting this to TRUE allows you to ignore this
 constraint.

 *When does it make sense to do this?
  XModem assumes that all files could contain more data blocks than can be
  indexed with the packet id byte. If this does not hold true for your use case
  and you can map each part of the transfer file to a unique packet index say
  because your "file" is a fixed storage like a RAM chip then it is much more
  efficient to transfer only the blocks you need to.

void bufferPacketReads(bool)
 Setting this to FALSE forces the library to use the minimum memory possible
 at the expense of slower read speeds as it can only read packets 1 byte at a
 time instead of providing storage space to the serial device and processing
 the packet once all the data has arrived.

void setRecieveBlockHandler(Receive Block Handler)
 Receive Block Handler prototype: bool handler(void *blk_id, size_t idSize, byte *data, size_t dataSize)
 This allows you to set a custom callback function for processing received
 data, return TRUE will allow the transfer of the next block to continue while
 returning FALSE will cancel the transfer. A simple version of this could just
 copy the data into an array but a more advanced version might compare this to
 what is stored in a RAM chip and write the difference to it. If you plan to
 receive data then you need to set this as the default Receive Block Handler
 simply discards all data.

void setBlockLookupHandler(Block Lookup Handler)
 Block Lookup Handler prototype: void handler(void *blk_id, size_t idSize, byte *send_data, size_t dataSize)
 This allows you to use an external data storage by using the blk_id as an
 lookup index/address and loading the dataSize bytes into the send_data pointer.
 This avoids needing to preload a full transactions worth of data in memory
 before sending. If you plan to send data this way you will need to set this
 and use the lookup_send method or the send_bulk_data method with every id
 that should be looked up corresponding to a NULL data pointer and 0 len value.
 The default Block Lookup Handler fills the send_data pointer memory with the
 byte 0x3A (the colon character ':').

void setChksumHandler(Checksum Handler)
 Checksum Handler prototype: void handler(byte *data, size_t dataSize, byte *chksum)
 This allows you to set a custom callback function for calculating a XModem
 packets checksum. The calculated checksum is expected to be stored in the
 chksum variable for use by the library code. This is used both when sending
 and receiving data. Depending on your XModem::ProtocolType this will be be set
 to either XModem::basic_chksum or XModem::crc_16_chksum. A custom function for
 this is unlikely to be needed except in advanced use cases.

bool send_bulk_data(Bulk Data Struct)
 Start attempting to send the data in the Bulk Data Struct. Returns TRUE when
 the transfer has completed succesfully and FALSE if an error occured. This is
 not supported by the standard XModem protocol and will typically require Allow
 NonSequential Blocks to be set to TRUE. This is for advanced use cases such as
 if you know the current state of a file on the receiving end and would like to
 send updated blocks only where there have been changes. It is possible to do
 this with the normal send() methods but you have to do a seperate transfer for
 each contiguous block while this will pack it into a single transfer.

 Bulk Data Struct:
 This structure contains the following members:
  byte **data_arr   - An array of pointers to the data blocks to send, if there are
                      any NULL pointers then the corresponding packet id will be passed
                      to the Block Lookup Handler to retrieve the data that will be sent
  size_t *len_array - An array of the lengths of each data block
  byte *id_arr      - An array of the starting XModem packet id of each data
                      block, each id is expected to be ID Size bytes long and
                      in big endian format
  size_t count      - The number of data blocks in this struct

 *NOTE
  This library's receive implementation tracks duplicate blocks by keeping track
  of the previously sent block_id which is initialized to 0 since the expected
  first block_id for XModem is 1. So if you need to transmit the 0 block_id then
  make sure to transmit another block_id first.

KNOWN EDGE CASES

XModem packets that happen to end in 0x1A (SUB) bytes will be interpreted as
padding bytes at the receiving end and passed to the Recieve Block Handler with
a smaller dataSize parameter. This can cause some confusion especially when
sending raw binary data.
There are a number of possible workarounds depending on your situation:
 - If you have another non SUB byte sequence that you know can't occur in your data
   then you could inject it into the XModem packets that are sent (effectively
   making your packet size smaller) as a sentinel that the receiving end will
   know to discard.
 - If you can ensure that packets will always be completely filled you can
   ignore the dataSize parameter and access the data buffer as if dataSize
   was set to the full packet size. The data buffer always contains all the
   data that was received including the padding bytes.
 - If you are using the packet block id's in a way that maps one-to-one to
   some physical/logical destination and have extra unused block id's, you can
   use one of them as a communication or control page that tells the receiving
   end to expect X bytes of raw binary data

PERFORMANCE

Disregarding line corruption (bit-flips), reliable communication is dependent
on the receiving device preventing its serial input buffers from overflowing
as many serial connections discard incoming data in that situation.^

As such knowing approximately how fast a given configuration can run recieve
data without using trial and error is very helpful so I have recorded the
maximum stable recieve() baud rates observed using a simple sketch.

#TODO: We need to formalize this test case
_________________________________________________________________________________________________________
|                    |                                   |          |      |           bytes            |
|         name       |                fqbn               | buffered | baud |----------------------------|
|                    |                                   |          | rate | packet | id | chksm | data |
|--------------------|-----------------------------------|----------|------|--------|----|-------|------|
| Generic nano board | arduino:avr:nano:cpu=atmega328old |     Y    |  4800|      70|   2|      2|    64|
| Generic nano board | arduino:avr:nano:cpu=atmega328old |     N    |  1200|      70|   2|      2|    64|
| Generic nano board | arduino:avr:nano:cpu=atmega328old |     N    |  9600|      67|   1|      1|    64|
---------------------------------------------------------------------------------------------------------

^ Even when a serial connection uses software flow control it is often not
possible to prevent an overflow situation.

PORTS

Ports are in the extras/ports folder. Current ports are:

- linux C implementation

FUTURE WORK

- Automatic tests using https://github.com/Arduino-CI/arduino_ci or similar
- Allow SoftwareSerial or HardwareSerial to be used interchangeably?
- CRC_XMODEM needs debugging, trying to test using a basic sketch and lrzsz
  I need a way to spy on what lrzsz is sending/receiving
- Raspberry Pi Pico (RP2040) port
- A standardised performance testing sketch
