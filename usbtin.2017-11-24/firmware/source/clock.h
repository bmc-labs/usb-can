/********************************************************************
 File: clock.h

 Description:
 This file contains the clock definitions.

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

#ifndef _CLOCK_
#define _CLOCK_

extern void clock_init();
extern void clock_process();
extern unsigned short clock_getMS();
extern void clock_reset();

#define CLOCK_TIMERTICKS_1MS 375
#define CLOCK_TIMERTICKS_100MS 37500


#endif
