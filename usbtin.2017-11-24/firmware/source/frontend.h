/********************************************************************
 File: frontend.h

 Description:
 This file contains the frontend interface definitions.

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
#ifndef _FRONTEND_
#define _FRONTEND_

#define LINE_MAXLEN 100
#define BELL 7
#define CR 13
#define LR 10

#define RX_STEP_TYPE 0
#define RX_STEP_ID_EXT 1
#define RX_STEP_ID_STD 6
#define RX_STEP_DLC 9
#define RX_STEP_DATA 10
#define RX_STEP_TIMESTAMP 26
#define RX_STEP_CR 30
#define RX_STEP_FINISHED 0xff

unsigned char transmitStd(char *line);
void parseLine(char * line);
char canmsg2ascii_getNextChar(canmsg_t * canmsg, unsigned char * step);
void sendbuffer_send();
unsigned char sendbuffer_isEmpty();
void sendStatusflags(unsigned char sendeol);
void frontend_sendErrorflags(unsigned char flags);

#endif
