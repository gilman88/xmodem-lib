Linux C implementation of the arduino XModem Transfer library

This was mostly built as a debugging platform for working out where the
arduino implementation was going wrong. Check xmodem.h for the public methods.
It does change a few things in the interface like using compile time defines
for things like XMODEM_ALLOW_NONSEQUENTIAL, and XMODEM_BUFFER_PACKET_READS
To help with debugging you can define the following:
  XMODEM_DEBUG          - This prints out protocol information like packets that are sent or received
  XMODEM_RESPONSE_DEBUG - If XMODEM_DEBUG is also defined then this will print out bytes that are
                          recieved when waiting for signals between packets.
