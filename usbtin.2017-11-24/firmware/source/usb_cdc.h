/********************************************************************
 File: usb_cdc.h

 Description:
 This file contains the USB CDC definitions.

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
#ifndef _USB_CDC_
#define _USB_CDC_

extern void usb_putch(unsigned char ch);
extern void usb_putstr(char * s);
extern unsigned char usb_chReceived();
extern unsigned char usb_getch();
extern void usb_init();
extern void usb_process();
extern void usb_txprocess();
unsigned char usb_ep1_ready();
void usb_ep1_flush();
unsigned char usb_serialNumberAvailable();
unsigned char usb_isConfigured();

const unsigned char usb_string_serial[] @ 0x0300;

#endif
