/********************************************************************
 File: main.c

 Description:
 This file contains the main logic of the USBtin project.
 USBtin is a simple USB to CAN interface. It uses the USB class CDC
 to connect to the host. Configuration and bus communication is
 handled over this CDC virtual comport.

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
 
 ********************************************************************/

#include <htc.h>
#include <stdio.h>
#include <string.h>
#include "usb_cdc.h"
#include "clock.h"
#include "mcp2515.h"
#include "frontend.h"
#include "usbtin.h"

volatile unsigned char state = STATE_CONFIG;


/**
 * Main function. Entry point for USBtin application.
 * Handles initialization and the the main processing loop.
 */
void main(void) {

    // disable all analog pin functions, set led pin to output
    ANSEL = 0;
    ANSELH = 0;
    TRISBbits.TRISB5 = 0;
    hardware_setLED(0);

    // initialize MCP2515 (reset and clock setup)
    unsigned char selftest = mcp2515_init();
   
    // switch (back) to external clock (if fail-safe-monitor switched to internal)
    OSCCON = 0x30;

    // initialize modules
    clock_init();
    usb_init();
    
    // buffer for incoming characters
    char line[LINE_MAXLEN];
    unsigned char linepos = 0;
    
    canmsg_t canmsg_buffer[CANMSG_BUFFERSIZE];
    unsigned char canmsg_buffer_filled = 0;
    unsigned char canmsg_buffer_canpos = 0;
    unsigned char canmsg_buffer_usbpos = 0;
    unsigned char rxstep = 0;
    
    unsigned short led_lastclock = TMR0;
    unsigned char led_ticker = 0;
    unsigned char reportstatus_timeout = 0;
    unsigned char reportedStatus = 0;


    // main loop
    while (1) {

        // do module processing
        usb_process();
        clock_process();

        // handles rx interrupt requests of MCP2515 controller: receive message and store it to buffer
        while ((state != STATE_CONFIG) && (mcp2515_getPinstateRX0BF() || mcp2515_getPinstateRX1BF()) && (canmsg_buffer_filled < CANMSG_BUFFERSIZE) && (mcp2515_receive_message(&canmsg_buffer[canmsg_buffer_canpos]))) {
                canmsg_buffer_canpos = (canmsg_buffer_canpos + 1) % CANMSG_BUFFERSIZE;
                canmsg_buffer_filled++;                            
        }        

        if (!sendbuffer_isEmpty() && (rxstep == 0)) {

            while (usb_ep1_ready() && !sendbuffer_isEmpty()) {
                sendbuffer_send();
            }
            
        } else {

            // process can messages in receive buffer
            while (usb_ep1_ready() && (canmsg_buffer_filled > 0)) {
                usb_putch(canmsg2ascii_getNextChar(&canmsg_buffer[canmsg_buffer_usbpos], &rxstep));
                if (rxstep == RX_STEP_FINISHED) {
                    // finished this frame
                    rxstep = 0;
                    canmsg_buffer_usbpos = (canmsg_buffer_usbpos + 1) % CANMSG_BUFFERSIZE;
                    canmsg_buffer_filled--;
                    break;
                }
            }

        }
        
        // receive characters from virtual serial port and collect the data until end of line is indicated
        while (usb_chReceived() && (rxstep == 0) && sendbuffer_isEmpty()) {
            unsigned char ch = usb_getch();

            if (ch == CR) {
                line[linepos] = 0;
                parseLine(line);
                linepos = 0;
            } else if (ch != LR) {
                line[linepos] = ch;
                if (linepos < LINE_MAXLEN - 1) linepos++;
            }
        }

        // handle error interrupt
        if ((mcp2515_getPinstateInt() || ((reportedStatus != 0) && (reportstatus_timeout == 0))) && (rxstep == 0) && sendbuffer_isEmpty()) {
           
           unsigned char flags = mcp2515_read_errorflags();
           mcp2515_bit_modify(MCP2515_REG_CANINTF, 0x20, 0);
           
           if (flags != reportedStatus) {
               reportedStatus = flags;
               frontend_sendErrorflags(flags);
           }
           
           reportstatus_timeout = 20; // 2s
        }

        // led signaling        
        if ((unsigned short) (TMR0 - led_lastclock) > CLOCK_TIMERTICKS_100MS) {
            led_lastclock += CLOCK_TIMERTICKS_100MS;
            led_ticker++;            
            if (reportstatus_timeout) reportstatus_timeout--;
        }
        if (!selftest) {
            // selftest failed
            hardware_setLED((led_ticker % 4 == 0) && (led_ticker % 32 != 0));
        } else if (!usb_isConfigured()) {    
            // usb is not enumerated/configured
            hardware_setLED((led_ticker % 4 == 0) && (led_ticker % 16 != 0));
        } else {
            hardware_setLED((led_ticker % 16 == 0) || (state != STATE_CONFIG));
        }

        // jump into bootloader, if jumper is closed
        if (hardware_getBLSwitch()) {
            UCON = 0;
            _delay(1000);
            RESET();
        }
    }
}
