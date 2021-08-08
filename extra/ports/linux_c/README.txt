Linux C implementation of the arduino XModem Transfer library

This was mostly built as a debugging platform for working out where the
arduino implementation was going wrong.
It does change a few things in the interface like using compile time defines
for things like XMODEM_DEBUG, XMODEM_ALLOW_NONSEQUENTIAL, and XMODEM_BUFFER_PACKET_READS
as well as including extra things like a method to initialize a serial file
descriptor with the provided baud rate so you only need to pass in the path to
your serial device, check xmodem.h for the public methods.
