/********************************************************************
 File: mcp2515.h

 Description:
 This file contains the MCP2515 interface definitions.

 Authors and Copyright:
 (c) 2012-2016, Thomas Fischl <tfischl@gmx.de>

 Device: PIC18F14K50
 Compiler: Microchip MPLAB XC8 C Compiler V1.37

 License:
 This file is open source. You can use it or parts of it in own
 open source projects. For closed or commercial projects you have to
 contact the authors listed above to get the permission for using
 this source code.

 ********************************************************************/
#ifndef _MCP2515_
#define _MCP2515_

// standard timing definitions
#define MCP2515_TIMINGS_10K  0xfb, 0xad, 0x06	// PropSeg=6Tq, PS1=6Tq, PS2=7Tq, SamplePoint=65%, SJW=4
#define MCP2515_TIMINGS_20K  0xdd, 0xad, 0x06   // PropSeg=6Tq, PS1=6Tq, PS2=7Tq, SamplePoint=65%, SJW=4
#define MCP2515_TIMINGS_50K  0xcb, 0xad, 0x06   // PropSeg=6Tq, PS1=6Tq, PS2=7Tq, SamplePoint=65%, SJW=4
#define MCP2515_TIMINGS_100K 0xc5, 0xad, 0x06   // PropSeg=6Tq, PS1=6Tq, PS2=7Tq, SamplePoint=65%, SJW=4
#define MCP2515_TIMINGS_125K 0xc5, 0xa4, 0x04   // PropSeg=5Tq, PS1=5Tq, PS2=5Tq, SamplePoint=68.75%, SJW=4
#define MCP2515_TIMINGS_250K 0xc2, 0xa4, 0x04	// PropSeg=5Tq, PS1=5Tq, PS2=5Tq, SamplePoint=68.75%, SJW=4
#define MCP2515_TIMINGS_500K 0xc1, 0x9a, 0x03   // PropSeg=3Tq, PS1=4Tq, PS2=4Tq, SamplePoint=66.67%, SJW=4
#define MCP2515_TIMINGS_800K 0xc0, 0xa3, 0x04   // PropSeg=4Tq, PS1=5Tq, PS2=5Tq, SamplePoint=66.67%, SJW=4
#define MCP2515_TIMINGS_1M   0xc0, 0x9a, 0x03   // PropSeg=4Tq, PS1=3Tq, PS2=4Tq, SamplePoint=66.67%, SJW=4

// pin mapping
#define MCP2515_SS LATCbits.LATC6
#define mcp2515_getPinstateInt() !PORTCbits.RC2
#define mcp2515_getPinstateRX0BF() !PORTCbits.RC3
#define mcp2515_getPinstateRX1BF() !PORTBbits.RB7

// command definitions
#define MCP2515_CMD_RESET 0xC0
#define MCP2515_CMD_READ 0x03
#define MCP2515_CMD_WRITE 0x02
#define MCP2515_CMD_BIT_MODIFY 0x05
#define MCP2515_CMD_READ_STATUS 0xA0
#define MCP2515_CMD_LOAD_TX 0x40
#define MCP2515_CMD_RTS 0x80
#define MCP2515_CMD_RX_STATUS 0xB0
#define MCP2515_CMD_READ_RX 0x90

// register definitions
#define MCP2515_REG_CNF1 0x2A
#define MCP2515_REG_CNF2 0x29
#define MCP2515_REG_CNF3 0x28
#define MCP2515_REG_CANCTRL 0x0F
#define MCP2515_REG_RXB0CTRL 0x60
#define MCP2515_REG_RXB1CTRL 0x70
#define MCP2515_REG_BFPCTRL 0x0C
#define MCP2515_REG_CANINTF 0x2C
#define MCP2515_REG_CANINTE 0x2B
#define MCP2515_REG_TXB0CTR 0x30
#define MCP2515_REG_TXB1CTR 0x40
#define MCP2515_REG_TXB2CTR 0x50

#define MCP2515_REG_RXF0SIDH 0x00
#define MCP2515_REG_RXF0SIDL 0x01
#define MCP2515_REG_RXF1SIDH 0x04
#define MCP2515_REG_RXF1SIDL 0x05
#define MCP2515_REG_RXF2SIDH 0x08
#define MCP2515_REG_RXF2SIDL 0x09
#define MCP2515_REG_RXF3SIDH 0x10
#define MCP2515_REG_RXF3SIDL 0x11
#define MCP2515_REG_RXF4SIDH 0x14
#define MCP2515_REG_RXF4SIDL 0x15
#define MCP2515_REG_RXF5SIDH 0x18
#define MCP2515_REG_RXF5SIDL 0x19

#define MCP2515_REG_RXM0SIDH 0x20
#define MCP2515_REG_RXM0SIDL 0x21
#define MCP2515_REG_RXM0EID8 0x22
#define MCP2515_REG_RXM0EID0 0x23
#define MCP2515_REG_RXM1SIDH 0x24
#define MCP2515_REG_RXM1SIDL 0x25
#define MCP2515_REG_RXM1EID8 0x26
#define MCP2515_REG_RXM1EID0 0x27
#define MCP2515_REG_EFLG 0x2d


// can message data structure
typedef struct
{
    unsigned long id; 			// identifier (11 or 29 bit)
    struct {
       unsigned char rtr : 1;		// remote transmit request
       unsigned char extended : 1;	// extended identifier
    } flags;

    unsigned char dlc;                  // data length code
    unsigned char data[8];		// payload data
    unsigned short timestamp;           // timestamp
} canmsg_t;

// function prototypes
extern unsigned char mcp2515_init();
extern unsigned char mcp2515_read_register(unsigned char address);
extern void mcp2515_write_register(unsigned char address, unsigned char data);
extern void mcp2515_bit_modify(unsigned char address, unsigned char mask, unsigned char data);
extern void mcp2515_set_SJA1000_filter_mask(unsigned char amr0, unsigned char amr1, unsigned char amr2, unsigned char amr3);
extern void mcp2515_set_SJA1000_filter_code(unsigned char acr0, unsigned char acr1, unsigned char acr2, unsigned char acr3);
extern unsigned char mcp2515_read_errorflags();
extern void mcp2515_set_bittiming(unsigned char cnf1, unsigned char cnf2, unsigned char cnf3);
extern unsigned char mcp2515_send_message(canmsg_t * p_canmsg);
extern unsigned char mcp2515_rx_status();
extern unsigned char mcp2515_receive_message(canmsg_t * p_canmsg);


#endif
