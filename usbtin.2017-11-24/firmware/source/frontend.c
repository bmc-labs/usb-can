/********************************************************************
 File: frontend.c

 Description:
 This file contains the frontend interface functions.

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
#include "usb_cdc.h"
#include "mcp2515.h"
#include "clock.h"
#include "usbtin.h"
#include "frontend.h"

unsigned char timestamping = 0;

#define SENDBUFFER_MAXSIZE 8
unsigned char sendbuffer[SENDBUFFER_MAXSIZE];
unsigned char sendbuffer_size = 0;
unsigned char sendbuffer_tx_pos = 0;

unsigned char sendbuffer_isEmpty() {
    return sendbuffer_size == 0;
}

void sendbuffer_send() {

    if (sendbuffer_tx_pos < sendbuffer_size) {

        usb_putch(sendbuffer[sendbuffer_tx_pos]);
        sendbuffer_tx_pos++;

    } else {
        // completed
        sendbuffer_tx_pos = 0;
        sendbuffer_size = 0;
    }
}

void sendbuffer_putch(unsigned char ch) {

    if (sendbuffer_size == SENDBUFFER_MAXSIZE) {
        return;
    }

    sendbuffer[sendbuffer_size] = ch;
    sendbuffer_size++;
}




/**
 * Parse hex value of given string
 *
 * @param line Input string
 * @param len Count of characters to interpret
 * @param value Pointer to variable for the resulting decoded value
 * @return 0 on error, 1 on success
 */
unsigned char parseHex(char * line, unsigned char len, unsigned long * value) {
    *value = 0;
    while (len--) {
        if (*line == 0) return 0;
        *value <<= 4;
        if ((*line >= '0') && (*line <= '9')) {
           *value += *line - '0';
        } else if ((*line >= 'A') && (*line <= 'F')) {
           *value += *line - 'A' + 10;
        } else if ((*line >= 'a') && (*line <= 'f')) {
           *value += *line - 'a' + 10;
        } else return 0;
        line++;
    }
    return 1;
}

/**
 * Send given value as hexadecimal string
 *
 * @param value Value to send as hex over the UART
 * @param len Count of characters to produce
 */
/*
void sendHex(unsigned long value, unsigned char len) {

    char s[20];
    s[len] = 0;

    while (len--) {

        unsigned char hex = value & 0x0f;
        if (hex > 9) hex = hex - 10 + 'A';
        else hex = hex + '0';
        s[len] = hex;

        value = value >> 4;
    }

    usb_putstr(s);

}
*/

/**
 * Send given byte value as hexadecimal string
 *
 * @param value Byte value to send over UART
 */
void sendByteHex(unsigned char value) {

//    sendHex(value, 2);
    
    unsigned char ch = value >> 4;
    if (ch > 9) ch = ch - 10 + 'A';
    else ch = ch + '0';
    sendbuffer_putch(ch);

    ch = value & 0xF;
    if (ch > 9) ch = ch - 10 + 'A';
    else ch = ch + '0';
    sendbuffer_putch(ch);
    
}

/**
 * Interprets given line and transmit can message
 *
 * @param line Line string which contains the transmit command
 */
unsigned char parseCmd_transmit(char *line) {
    canmsg_t canmsg;
    unsigned long temp;
    unsigned char idlen;

    canmsg.flags.rtr = ((line[0] == 'r') || (line[0] == 'R'));

    // upper case -> extended identifier
    if (line[0] < 'Z') {
        canmsg.flags.extended = 1;
        idlen = 8;
    } else {
        canmsg.flags.extended = 0;
        idlen = 3;
    }

    if (!parseHex(&line[1], idlen, &temp)) return 0;
    canmsg.id = temp;

    if (!parseHex(&line[1 + idlen], 1, &temp)) return 0;
    canmsg.dlc = temp;

    if (!canmsg.flags.rtr) {
        unsigned char i;
        unsigned char length = canmsg.dlc;
        if (length > 8) length = 8;
        for (i = 0; i < length; i++) {
            if (!parseHex(&line[idlen + 2 + i*2], 2, &temp)) return 0;
            canmsg.data[i] = temp;
        }
    }

    return mcp2515_send_message(&canmsg);
}

/**
 * Interprets given line and set up bit timing
 *
 * @param line Line string which contains the transmit command
 */
unsigned char parseCmd_setupUserdefined(char * line) {
    
    if (state == STATE_CONFIG) {
        unsigned long cnf1, cnf2, cnf3;
        if (parseHex(&line[1], 2, &cnf1) && parseHex(&line[3], 2, &cnf2) && parseHex(&line[5], 2, &cnf3)) {
            mcp2515_set_bittiming(cnf1, cnf2, cnf3);
            return CR;
        }
    } 
    return BELL;
}

/**
 * Interprets given line and reads register from MCP2515
 *
 * @param line Line string which contains the transmit command
 */
unsigned char parseCmd_readRegister(char * line) {
    
    unsigned long address;
    if (parseHex(&line[1], 2, &address)) {
        unsigned char value = mcp2515_read_register(address);
        sendByteHex(value);
        return CR;
    }
    return BELL;
}

/**
 * Interprets given line and writes MCP2515 register
 *
 * @param line Line string which contains the transmit command
 */
unsigned char parseCmd_writeRegister(char * line) {
        
    unsigned long address, data;
    if (parseHex(&line[1], 2, &address) && parseHex(&line[3], 2, &data)) {
        mcp2515_write_register(address, data);
        return CR;
    }
    return BELL;
}

/**
 * Interprets given line and set time stamping
 *
 * @param line Line string which contains the transmit command
 */
unsigned char parseCmd_setTimestamping(char * line) {
    
    unsigned long stamping;
    if (parseHex(&line[1], 1, &stamping)) {
        timestamping = (stamping != 0);
        return CR;
    }
    
    return BELL;
}

/**
 * Send out given error flags
 * 
 * @param flags Error flags to send out
 */
void frontend_sendErrorflags(unsigned char flags) {
    
    sendbuffer_putch('F');
    sendByteHex(flags);
    sendbuffer_putch(CR);
}

/**
 * Interprets given line and handle status flag requests
 *
 * @param line Line string which contains the transmit command
 */
unsigned char parseCmd_errorFlags(char * line) {

    unsigned char flags = mcp2515_read_errorflags();

    sendbuffer_putch('F');
    sendByteHex(flags);
    return CR;
}

/**
 * Interprets given line and handle error reporting requests
 *
 * @param line Line string which contains the transmit command
 */
unsigned char parseCmd_errorReporting(char * line) {

    unsigned long subcmd = 0;
    if (parseHex(&line[1], 1, &subcmd)) {

        switch (subcmd) {
            case 0x0: // Disable status reporting
                mcp2515_write_register(MCP2515_REG_CANINTE, 0x00);
                return CR;
            case 0x1: // Enable status reporting
                mcp2515_write_register(MCP2515_REG_CANINTE, 0x20); // ERRIE interrupt to INT pin
                return CR;
            case 0x2: // Clear overrun errors
                mcp2515_write_register(MCP2515_REG_EFLG, 0x00);
                return CR;        
            case 0x3: // Reinit/reset MCP2515 to clear all errors
                if (state == STATE_CONFIG) {
                    mcp2515_init();
                    return CR;
                }
                break;
        }
    }
    
    return BELL;    
}

/**
 * Interprets given line and set filter mask
 *
 * @param line Line string which contains the transmit command
 */
unsigned char parseCmd_setFilterMask(char * line) {
    if (state == STATE_CONFIG)
    {
        unsigned long am0, am1, am2, am3;
        if (parseHex(&line[1], 2, &am0) && parseHex(&line[3], 2, &am1) && parseHex(&line[5], 2, &am2) && parseHex(&line[7], 2, &am3)) {
            mcp2515_set_SJA1000_filter_mask(am0, am1, am2, am3);
            return CR;
        }
    } 
    
    return BELL;
}

/**
 * Interprets given line and set filter code
 *
 * @param line Line string which contains the transmit command
 */
unsigned char parseCmd_setFilterCode(char * line) {
    if (state == STATE_CONFIG)
    {
        unsigned long ac0, ac1, ac2, ac3;
        if (parseHex(&line[1], 2, &ac0) && parseHex(&line[3], 2, &ac1) && parseHex(&line[5], 2, &ac2) && parseHex(&line[7], 2, &ac3)) {
            mcp2515_set_SJA1000_filter_code(ac0, ac1, ac2, ac3);
            return CR;
        }
    } 
    return BELL;
}

/**
 * Interprets given line and jump to bootloader
 *
 * @param line Line string which contains the transmit command
 */
unsigned char parseCmd_bootloaderJump(char * line) {
    
    unsigned long magic;
    if (parseHex(&line[1], 2, &magic)) {
                    
        // check magic code and if bootloader version is new enough to supports bootloader entry via jump
        if ((magic == 0x10) && usb_serialNumberAvailable()) {

            usb_putch(CR);
            usb_ep1_flush();

            unsigned char delaycycles = 10;
            while (delaycycles--) {
                if (delaycycles < 5) UCON = 0; // turn off USB

                hardware_setLED(delaycycles & 1);

                unsigned short ticker = TMR0;                        
                while ((unsigned short) (TMR0 - ticker) < CLOCK_TIMERTICKS_100MS) {
                    usb_process();
                }
            }
            hardware_setLED(0);                    

            #asm
                goto BOOTLOADER_ENTRY_ADDRESS
            #endasm
        }             
    }
            
    return BELL;
}


/**
 * Parse given command line
 *
 * @param line Line string to parse
 */
void parseLine(char * line) {

    unsigned char result = BELL;
    
    switch (line[0]) {
        case 'S': // Setup with standard CAN bitrates
            if (state == STATE_CONFIG)
            {
                switch (line[1]) {
                    case '0': mcp2515_set_bittiming(MCP2515_TIMINGS_10K);  result = CR; break;
                    case '1': mcp2515_set_bittiming(MCP2515_TIMINGS_20K);  result = CR; break;
                    case '2': mcp2515_set_bittiming(MCP2515_TIMINGS_50K);  result = CR; break;
                    case '3': mcp2515_set_bittiming(MCP2515_TIMINGS_100K); result = CR; break;
                    case '4': mcp2515_set_bittiming(MCP2515_TIMINGS_125K); result = CR; break;
                    case '5': mcp2515_set_bittiming(MCP2515_TIMINGS_250K); result = CR; break;
                    case '6': mcp2515_set_bittiming(MCP2515_TIMINGS_500K); result = CR; break;
                    case '7': mcp2515_set_bittiming(MCP2515_TIMINGS_800K); result = CR; break;
                    case '8': mcp2515_set_bittiming(MCP2515_TIMINGS_1M);   result = CR; break;
                }

            }
            break;
        case 's': // Setup with user defined timing settings for CNF1/CNF2/CNF3
            result = parseCmd_setupUserdefined(line);
            break;
        case 'G': // Read given MCP2515 register
            result = parseCmd_readRegister(line);
            break;
        case 'W': // Write given MCP2515 register
            result = parseCmd_writeRegister(line);
            break;
        case 'V': // Get hardware version
            {

                sendbuffer_putch('V');
                sendByteHex(VERSION_HARDWARE_MAJOR);
                sendByteHex(VERSION_HARDWARE_MINOR);
                result = CR;
            }
            break;
        case 'v': // Get firmware version
            {
                
                sendbuffer_putch('v');
                sendByteHex(VERSION_FIRMWARE_MAJOR);
                sendByteHex(VERSION_FIRMWARE_MINOR);
                result = CR;
            }
            break;
        case 'N': // Get serial number
            {
                sendbuffer_putch('N');
                if (usb_serialNumberAvailable()) {
                    sendbuffer_putch(usb_string_serial[10]);
                    sendbuffer_putch(usb_string_serial[12]);
                    sendbuffer_putch(usb_string_serial[14]);
                    sendbuffer_putch(usb_string_serial[16]);
                } else {
                    sendbuffer_putch('F');
                    sendbuffer_putch('F');
                    sendbuffer_putch('F');
                    sendbuffer_putch('F');
                }
                result = CR;
            }
            break;     
        case 'O': // Open CAN channel
            if (state == STATE_CONFIG)
            {
		mcp2515_bit_modify(MCP2515_REG_CANCTRL, 0xE0, 0x00); // set normal operating mode

                clock_reset();

                state = STATE_OPEN;
                result = CR;
            }
            break; 
        case 'l': // Loop-back mode
            if (state == STATE_CONFIG)
            {
		mcp2515_bit_modify(MCP2515_REG_CANCTRL, 0xE0, 0x40); // set loop-back

                state = STATE_OPEN;
                result = CR;
            }
            break; 
        case 'L': // Open CAN channel in listen-only mode
            if (state == STATE_CONFIG)
            {
		mcp2515_bit_modify(MCP2515_REG_CANCTRL, 0xE0, 0x60); // set listen-only mode

                state = STATE_LISTEN;
                result = CR;
            }
            break; 
        case 'C': // Close CAN channel
            if (state != STATE_CONFIG)
            {
		mcp2515_bit_modify(MCP2515_REG_CANCTRL, 0xE0, 0x80); // set configuration mode

                state = STATE_CONFIG;
                result = CR;
            }
            break; 
        case 'r': // Transmit standard RTR (11 bit) frame
        case 'R': // Transmit extended RTR (29 bit) frame
        case 't': // Transmit standard (11 bit) frame
        case 'T': // Transmit extended (29 bit) frame
            if (state == STATE_OPEN)
            {
                if (parseCmd_transmit(line)) {
                    if (line[0] < 'Z') sendbuffer_putch('Z');
                    else sendbuffer_putch('z');
                    result = CR;
                }

            }
            break;        
        case 'f': // Handle error reporting requests
            result = parseCmd_errorReporting(line);
            break;
        case 'F': // Handle status flag requests
            result = parseCmd_errorFlags(line);
            break;
        case 'Z': // Set time stamping
            result = parseCmd_setTimestamping(line);
            break;
        case 'm': // Set accpetance filter mask
            result = parseCmd_setFilterMask(line);
            break;
        case 'M': // Set accpetance filter code
            result = parseCmd_setFilterCode(line);
            break;
        case 'B': // Jump to bootloader
            result = parseCmd_bootloaderJump(line);
            break;
 
    }

   sendbuffer_putch(result);
}

/**
 * Get next character of given can message in ascii format
 * 
 * @param canmsg Pointer to can message
 * @param step Current step
 * @return Next character to print out
 */
char canmsg2ascii_getNextChar(canmsg_t * canmsg, unsigned char * step) {
    
    char ch = BELL;
    char newstep = *step;       
    
    if (*step == RX_STEP_TYPE) {
        
            // type
             
            if (canmsg->flags.extended) {                 
                newstep = RX_STEP_ID_EXT;                
                if (canmsg->flags.rtr) ch = 'R';
                else ch = 'T';
            } else {
                newstep = RX_STEP_ID_STD;
                if (canmsg->flags.rtr) ch = 'r';
                else ch = 't';
            }        
             
    } else if (*step < RX_STEP_DLC) {

	// id        

        unsigned char i = *step - 1;
        unsigned char * id_bp = (unsigned char*) &canmsg->id;
        ch = id_bp[3 - (i / 2)];
        if ((i % 2) == 0) ch = ch >> 4;
        
        ch = ch & 0xF;
        if (ch > 9) ch = ch - 10 + 'A';
        else ch = ch + '0';
        
        newstep++;
        
    } else if (*step < RX_STEP_DATA) {

	// length        

        ch = canmsg->dlc;
        
        ch = ch & 0xF;
        if (ch > 9) ch = ch - 10 + 'A';
        else ch = ch + '0';

        if ((canmsg->dlc == 0) || canmsg->flags.rtr) newstep = RX_STEP_TIMESTAMP;
        else newstep++;
        
    } else if (*step < RX_STEP_TIMESTAMP) {
        
        // data        

        unsigned char i = *step - RX_STEP_DATA;
        ch = canmsg->data[i / 2];
        if ((i % 2) == 0) ch = ch >> 4;
        
        ch = ch & 0xF;
        if (ch > 9) ch = ch - 10 + 'A';
        else ch = ch + '0';
        
        newstep++;        
        if (newstep - RX_STEP_DATA == canmsg->dlc*2) newstep = RX_STEP_TIMESTAMP;
        
    } else if (timestamping && (*step < RX_STEP_CR)) {
        
        // timestamp
        
        unsigned char i = *step - RX_STEP_TIMESTAMP;
        if (i < 2) ch = (canmsg->timestamp >> 8) & 0xff;
        else ch = canmsg->timestamp & 0xff;
        if ((i % 2) == 0) ch = ch >> 4;
        
        ch = ch & 0xF;
        if (ch > 9) ch = ch - 10 + 'A';
        else ch = ch + '0';
        
        newstep++;        
        
    } else {
        
        // linebreak
        
        ch = CR;
        newstep = RX_STEP_FINISHED;
    }
    
    *step = newstep;
    return ch;
}
