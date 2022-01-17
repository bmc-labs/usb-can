/********************************************************************
 File: usbtin.h

 Description:
 This file contains global project definitions.

 Authors and Copyright:
 (c) 2012-2017, Thomas Fischl (http://www.fischl.de/contact.html)

 Device: PIC18F14K50
 Compiler: Microchip MPLAB XC8 C Compiler V1.44

 License:
 This file is part of the USBtin firmware project and is copyrighted by the
 authors listed above. It is free for private or educational non-commercial use
 (for a commercial license please contact the authors). It may not be
 redistributed without the prior written consent of the authors.
 
 It is provided "as is" without warranty of any kind, either express or implied,
 including without limitation any implied warranties of condition, uninterrupted
 use, merchantability, fitness for a particular purpose, or non-infringement. In
 no event shall the authors be liable for any direct or indirect damages arising
 in any way out of the use of it. 

 Change History:
  Rev   Date        Description
  1.0   2011-12-18  Initial release
  1.1   2012-03-09  Minor fixes, code cleanup
  1.2   2013-01-11  Added filter function (command 'm' and 'M')
                    Added write register function (command 'W')
  1.3   2014-04-07  Changed/improved can bit timings
                    Fixed clock_process() (no blocking)
                    Fixed extended frames filtering
                    Added command 'V': return hardware version
                    Empty both can buffers at same loop cycle
                    Trigger USB sending in usb_putch
                    Increased SPI speed to 3 MHz (was 0,75 MHz)
                    Increased CDC buffer size to 128 (was 100)
                    Process received characters in a loop
  1.4   2014-09-20  Increased bulk transfer packet size (now 64)
  1.5   2014-12-05  New buffer structure, added CAN message buffer
                    Use USB ping-pong buffers
                    CDC/putch writes directly to USB ram
                    Printout of can messages within state-machine
                    Fixed handling of messages with DLC > 8
  1.6   2016-04-13  Fixed order of outgoing messages (TX fifo)
                    Added support for usb serial number string
                    Handle "halt endpoint" usb command
  1.7   2016-10-13  Fixed initialization of filter registers
                    Removed manual clearing of the RX flag
                    Changed main loop priorities
                    Enabled MCP2515 Rollover      
  1.8   2017-11-24  Fixed main loop priorities (process only one message per loop)
                    Fixed extended and rtr detection when both rx buffers are filled
                    Fixed rx buffer handling (was bug: readout one received rx multiple times)
                    Use three interrupt pins: two for RX, one for errorflags (was 1 common)
                    Incresed buffer for incoming can messages to 16 (was 8)
                    Added command to jump into bootloader (command 'B10')
                    Added command 'fx' for error status reporting
                    Added LED blinking (3x) if USB is not enumerated/configured
                    Added selftest. LED blinking on failure (7x)

 ********************************************************************/
#ifndef _USBTIN_
#define _USBTIN_

#define VERSION_HARDWARE_MAJOR 1
#define VERSION_HARDWARE_MINOR 0
#define VERSION_FIRMWARE_MAJOR 1
#define VERSION_FIRMWARE_MINOR 8

#define CANMSG_BUFFERSIZE 16

#define BOOTLOADER_ENTRY_ADDRESS 0x0030

#define STATE_CONFIG 0
#define STATE_OPEN 1
#define STATE_LISTEN 2

volatile unsigned char state;

#define hardware_setLED(value) LATBbits.LATB5 = value
#define hardware_getBLSwitch() !PORTAbits.RA3

#endif
