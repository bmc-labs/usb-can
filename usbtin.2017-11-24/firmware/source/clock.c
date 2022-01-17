/********************************************************************
 File: clock.c

 Description:
 This file contains the clock functions.

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
#include "clock.h"

unsigned short clock_lastclock;
unsigned short clock_msticker;

/**
 * Initialize clock/timer module
 */
void clock_init() {

    clock_reset();

    // set timer 0 to prescaler 1:32
    T0CON = 0x84;
}

/**
 * Handle clock task. Count milliseconds
 */
void clock_process() {
    if ((unsigned short) (TMR0 - clock_lastclock) > CLOCK_TIMERTICKS_1MS) {
       clock_lastclock += CLOCK_TIMERTICKS_1MS;
       clock_msticker++;
       if (clock_msticker > 60000) clock_msticker = 0;
    }
}

/**
 * Returns the milliseconds since last reset
 *
 * @return milliseconds since last reset
 */
unsigned short clock_getMS() {
    return clock_msticker;
}

/**
 * Reset millisecond counter
 */
void clock_reset() {
    TMR0 = 0;
    clock_lastclock = 0;
    clock_msticker = 0;
}
