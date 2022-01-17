/********************************************************************
 File: mcp2515.c

 Description:
 This file contains the MCP2515 interface functions.

 Authors and Copyright:
 (c) 2012-2017, Thomas Fischl <tfischl@gmx.de>

 Device: PIC18F14K50
 Compiler: Microchip MPLAB XC8 C Compiler V1.37

 License:
 This file is open source. You can use it or parts of it in own
 open source projects. For closed or commercial projects you have to
 contact the authors listed above to get the permission for using
 this source code.

 ********************************************************************/

#include <htc.h>
#include "usbtin.h"
#include "clock.h"
#include "mcp2515.h"

/** current transmit buffer priority */
unsigned char txprio = 3;

/** current rollover ping-pong buffer */
unsigned char current_rx_buffer = 0;

/**
 * \brief Transmit one byte over SPI bus
 *
 * \param c Character to send
 * \return Received byte
 */
unsigned char spi_transmit(unsigned char c) {
    SSPBUF = c;
    while (!SSPSTATbits.BF) {};
    return SSPBUF;
}


/**
 * \brief Write to given register
 *
 * \param address Register address
 * \param data Value to write to given register
 */
void mcp2515_write_register(unsigned char address, unsigned char data) {

    // pull SS to low level
    MCP2515_SS = 0;
   
    spi_transmit(MCP2515_CMD_WRITE);
    spi_transmit(address);
    spi_transmit(data);
   
    // release SS
    MCP2515_SS = 1;
}


/**
 * \brief Read from given register
 *
 * \param address Register address
 * \return register value
 */
unsigned char mcp2515_read_register(unsigned char address) {

    unsigned char data;
   
    // pull SS to low level
    MCP2515_SS = 0;
   
    spi_transmit(MCP2515_CMD_READ);
    spi_transmit(address);   
    data = spi_transmit(0xff); 
   
    // release SS
    MCP2515_SS = 1;
   
    return data;
}


/**
 * \brief Modify bit of given register
 *
 * \param address Register address
 * \param mask Mask of bits to set
 * \param data Values to set
 *
 * This function works only on a few registers. Please check the datasheet!
 */
void mcp2515_bit_modify(unsigned char address, unsigned char mask, unsigned char data) {

    // pull SS to low level
    MCP2515_SS = 0;
   
    spi_transmit(MCP2515_CMD_BIT_MODIFY);
    spi_transmit(address);
    spi_transmit(mask);
    spi_transmit(data);
   
    // release SS
    MCP2515_SS = 1;
}


/**
 * \brief Initialize spi interface, reset the MCP2515 and activate clock output signal
 */
unsigned char mcp2515_init() {

    unsigned char dummy;
    unsigned char selftest = 1;

    // init SPI
    SSPSTAT = 0x40; // CKE=1
    SSPCON1 = 0x21; // 3MHz SPI clock
    dummy = SSPBUF; // dummy read to clear BF
    dummy = 0;

    TRISBbits.TRISB4 = 1; // set TRIS of SDI
    TRISCbits.TRISC6 = 0; // clear TRIS of SS
    TRISCbits.TRISC7 = 0; // clear TRIS of SDO
    TRISBbits.TRISB6 = 0; // clear TRIS of SCK
    TRISCbits.TRISC3 = 1; // RX0BF
    TRISBbits.TRISB7 = 1; // RX1BF
    LATCbits.LATC6 = 1; // SS

    while (++dummy) {};

    // reset device
    LATCbits.LATC6 = 0; // SS
    spi_transmit(0xC0); // reset device
    LATCbits.LATC6 = 1; // SS

    while (++dummy) {};

    mcp2515_write_register(MCP2515_REG_CANCTRL, 0x85); // set config mode, clock prescaling 1:2 and clock output

    // configure filter
    mcp2515_write_register(MCP2515_REG_RXB0CTRL, 0x04); // use filter for standard and extended frames, enable rollover
    mcp2515_write_register(MCP2515_REG_RXB1CTRL, 0x00); // use filter for standard and extended frames

    // initialize filter mask
    mcp2515_write_register(MCP2515_REG_RXM0SIDH, 0x00);
    mcp2515_write_register(MCP2515_REG_RXM0SIDL, 0x00);
    mcp2515_write_register(MCP2515_REG_RXM0EID8, 0x00);
    mcp2515_write_register(MCP2515_REG_RXM0EID0, 0x00);
    mcp2515_write_register(MCP2515_REG_RXM1SIDH, 0x00);
    mcp2515_write_register(MCP2515_REG_RXM1SIDL, 0x00);
    mcp2515_write_register(MCP2515_REG_RXM1EID8, 0x00);
    mcp2515_write_register(MCP2515_REG_RXM1EID0, 0x00);

    mcp2515_write_register(MCP2515_REG_RXF0SIDL, 0x00);
    mcp2515_write_register(MCP2515_REG_RXF1SIDL, 0x08);
    mcp2515_write_register(MCP2515_REG_RXF2SIDL, 0x00);
    mcp2515_write_register(MCP2515_REG_RXF3SIDL, 0x08);

    // do basic self test
    mcp2515_write_register(MCP2515_REG_CANINTE, 0x00); // Disable all interrupts on INT pin
    mcp2515_write_register(MCP2515_REG_CANINTF, 0x00); // Clear interrupt flags
    mcp2515_write_register(MCP2515_REG_BFPCTRL, 0x0c); // switch RXnBF pins to digital mode    
    while (++dummy) {};    
    if (mcp2515_getPinstateInt() || !mcp2515_getPinstateRX0BF() || !mcp2515_getPinstateRX1BF()) selftest = 0;
    
    // test RXB0
    mcp2515_write_register(MCP2515_REG_BFPCTRL, 0x1c); // set RX0BF    
    while (++dummy) {};    
    if (mcp2515_getPinstateInt() || mcp2515_getPinstateRX0BF() || !mcp2515_getPinstateRX1BF()) selftest = 0;    
    
    // test RXB1
    mcp2515_write_register(MCP2515_REG_BFPCTRL, 0x2c); // set RX1BF
    while (++dummy) {};    
    if (mcp2515_getPinstateInt() || !mcp2515_getPinstateRX0BF() || mcp2515_getPinstateRX1BF()) selftest = 0;    
        
    // test INT
    mcp2515_write_register(MCP2515_REG_BFPCTRL, 0x0c); // clear RXnBF pins
    mcp2515_write_register(MCP2515_REG_CANINTF, 0x20); // Set error interrupt flag
    mcp2515_write_register(MCP2515_REG_CANINTE, 0x20); // Enable error interrupt
    while (++dummy) {};    
    if (!mcp2515_getPinstateInt() || !mcp2515_getPinstateRX0BF() || !mcp2515_getPinstateRX1BF()) selftest = 0;        
    
    // finish initialization
    mcp2515_write_register(MCP2515_REG_BFPCTRL, 0x0f); // RXnBF interrupts to pins
    mcp2515_write_register(MCP2515_REG_CANINTF, 0x00); // Clear interrupt flags
    mcp2515_write_register(MCP2515_REG_CANINTE, 0x00); // Disable all interrupts on INT pin
    
    return selftest;
}

/**
 * \brief Set filter mask of given SJA1000 register values
 *
 * \param amr0 Acceptence mask register 0
 * \param amr1 Acceptence mask register 1
 * \param amr2 Acceptence mask register 2
 * \param amr3 Acceptence mask register 3
 *
 * This function has only affect if mcp2515 is in configuration mode.
 * The filter mask is only set for the first 11 bit because of compatibility
 * issues between SJA1000 and MCP2515.
 */
void mcp2515_set_SJA1000_filter_mask(unsigned char amr0, unsigned char amr1, unsigned char amr2, unsigned char amr3) {

    // SJA1000 mask bit definition: 1 = accept without matching, 0 = do matching with acceptance code
    // MCP2515 mask bit definition: 0 = accept without matching, 1 = do matching with acceptance filter
    // -> invert mask

    // mask for filter 1
    mcp2515_write_register(MCP2515_REG_RXM0SIDH, ~amr0);
    mcp2515_write_register(MCP2515_REG_RXM0SIDL, (~amr1) & 0xE0);
    mcp2515_write_register(MCP2515_REG_RXM0EID8, 0x00);
    mcp2515_write_register(MCP2515_REG_RXM0EID0, 0x00);

    // mask for filter 2
    mcp2515_write_register(MCP2515_REG_RXM1SIDH, ~amr2);
    mcp2515_write_register(MCP2515_REG_RXM1SIDL, (~amr3) & 0xE0);
    mcp2515_write_register(MCP2515_REG_RXM1EID8, 0x00);
    mcp2515_write_register(MCP2515_REG_RXM1EID0, 0x00);

}

/**
 * \brief Set filter code of given SJA1000 register values
 *
 * \param amr0 Acceptence code register 0
 * \param amr1 Acceptence code register 1
 * \param amr2 Acceptence code register 2
 * \param amr3 Acceptence code register 3
 *
 * This function has only affect if mcp2515 is in configuration mode.
 */
void mcp2515_set_SJA1000_filter_code(unsigned char acr0, unsigned char acr1, unsigned char acr2, unsigned char acr3) {

    // acceptance code for filter 1
    mcp2515_write_register(MCP2515_REG_RXF0SIDH, acr0);
    mcp2515_write_register(MCP2515_REG_RXF0SIDL, (acr1) & 0xE0); // standard
    mcp2515_write_register(MCP2515_REG_RXF1SIDH, acr0);
    mcp2515_write_register(MCP2515_REG_RXF1SIDL, ((acr1) & 0xE0) | 0x08); // extended

    // acceptance code for filter 2
    mcp2515_write_register(MCP2515_REG_RXF2SIDH, acr2);
    mcp2515_write_register(MCP2515_REG_RXF2SIDL, (acr3) & 0xE0); // standard
    mcp2515_write_register(MCP2515_REG_RXF3SIDH, acr2);
    mcp2515_write_register(MCP2515_REG_RXF3SIDL, ((acr3) & 0xE0) | 0x08); // extended

    // fill remaining filters with zero
    mcp2515_write_register(MCP2515_REG_RXF4SIDH, 0x00);
    mcp2515_write_register(MCP2515_REG_RXF4SIDL, 0x00);
    mcp2515_write_register(MCP2515_REG_RXF5SIDH, 0x00);
    mcp2515_write_register(MCP2515_REG_RXF5SIDL, 0x00);
}


/**
 * \brief Set bit timing registers
 *
 * \param cnf1 Configuration register 1
 * \param cnf2 Configuration register 2
 * \param cnf3 Configuration register 3
 *
 * This function has only affect if mcp2515 is in configuration mode
 */
void mcp2515_set_bittiming(unsigned char cnf1, unsigned char cnf2, unsigned char cnf3) {

    mcp2515_write_register(MCP2515_REG_CNF1, cnf1);
    mcp2515_write_register(MCP2515_REG_CNF2, cnf2);
    mcp2515_write_register(MCP2515_REG_CNF3, cnf3);
}

/**
 * \brief Read status byte of MCP2515
 *
 * \return status byte of MCP2515
 */
unsigned char mcp2515_read_status() {

    // pull SS to low level
    MCP2515_SS = 0;
   
    spi_transmit(MCP2515_CMD_READ_STATUS);
    unsigned char status = spi_transmit(0xff);
   
    // release SS
    MCP2515_SS = 1;

    return status;
}

unsigned char mcp2515_read_errorflags() {
    
    unsigned char flags = mcp2515_read_register(MCP2515_REG_EFLG);
    unsigned char status = 0;

    if (flags & 0x01) status |= 0x04; // error warning
    if (flags & 0xC0) status |= 0x08; // data overrun
    if (flags & 0x18) status |= 0x20; // passive error
    if (flags & 0x20) status |= 0x80; // bus error
    
    return status;
}


/**
 * \brief Read RX status byte of MCP2515
 *
 * \return RX status byte of MCP2515
 */
unsigned char mcp2515_rx_status() {

    // pull SS to low level
    MCP2515_SS = 0;
   
    spi_transmit(MCP2515_CMD_RX_STATUS);
    unsigned char status = spi_transmit(0xff);
   
    // release SS
    MCP2515_SS = 1;

    return status;
}

/**
 * \brief Send given CAN message
 *
 * \ p_canmsg Pointer to can message to send
 * \return 1 if transmitted successfully to MCP2515 transmit buffer, 0 on error (= no free buffer available)
 */
unsigned char mcp2515_send_message(canmsg_t * p_canmsg) {

    unsigned char status = mcp2515_read_status();
    unsigned char address;
    unsigned char ctrlreg;
    unsigned char length;

    // check length
    length = p_canmsg->dlc;
    if (length > 8) length = 8;
    
    // do some priority fiddling to get fifo behavior
    switch (status & 0x54) {
        
        case 0x00:
            // all three buffers free
            ctrlreg = MCP2515_REG_TXB2CTR;
            address = 0x04;            
            txprio = 3;
            break;
            
        case 0x40:
        case 0x44:
            ctrlreg = MCP2515_REG_TXB1CTR;
            address = 0x02;            
            break;
            
        case 0x10:
        case 0x50:
            ctrlreg = MCP2515_REG_TXB0CTR;
            address = 0x00;
            break;
            
        case 0x04:
        case 0x14:         
            ctrlreg = MCP2515_REG_TXB2CTR;
            address = 0x04;
            
            if (txprio == 0) {
                // set priority of buffer 1 and buffer 0 to highest
                mcp2515_bit_modify(MCP2515_REG_TXB1CTR, 0x03, 0x03);
                mcp2515_bit_modify(MCP2515_REG_TXB0CTR, 0x03, 0x03);
                txprio = 2;
            } else {
                txprio--;
            }            
            break;
            
        default:
            // no free transmit buffer
            return 0;           
    }
    

    // pull SS to low level
    MCP2515_SS = 0;
   
    spi_transmit(MCP2515_CMD_LOAD_TX | address);

    if (p_canmsg->flags.extended) {
        spi_transmit(p_canmsg->id >> 21);
        spi_transmit(((p_canmsg->id >> 13) & 0xe0) | ((p_canmsg->id >> 16) & 0x03) | 0x08);
        spi_transmit(p_canmsg->id >> 8);
        spi_transmit(p_canmsg->id);
    } else {
        spi_transmit(p_canmsg->id >> 3);
        spi_transmit(p_canmsg->id << 5);
        spi_transmit(0);
        spi_transmit(0);
    }

    // length and data
    if (p_canmsg->flags.rtr) {
        spi_transmit(p_canmsg->dlc | 0x40);
    } else {
        spi_transmit(p_canmsg->dlc);
        unsigned char i;
        for (i = 0; i < length; i++) {
            spi_transmit(p_canmsg->data[i]);
        }
    }
   
    // release SS
    MCP2515_SS = 1;

    _delay(1);

    // request message to be transmitted
    mcp2515_write_register(ctrlreg, txprio | 0x08);
        
    return 1;
}

/*
 * \brief Read out one can message from MCP2515
 *
 * \param p_canmsg Pointer to can message structure to fill
 * \return 1 on success, 0 if there is no message to read
 */
unsigned char mcp2515_receive_message(canmsg_t * p_canmsg) {

    unsigned char address;    

    if (mcp2515_getPinstateRX0BF() && mcp2515_getPinstateRX1BF()) {
        // messages in both buffers
        address = (current_rx_buffer == 0) ? 0x00 : 0x04;
    } else if (mcp2515_getPinstateRX1BF()) {
        // message in RXB1
        address = 0x04;
        current_rx_buffer = 1;
    } else if (mcp2515_getPinstateRX0BF()) {
        // message in RXB0
        address = 0x00;
        current_rx_buffer = 0;
    } else {
        // no message in receive buffer
        return 0;
    }

    // store timestamp
    p_canmsg->timestamp = clock_getMS();        

    // pull SS to low level
    MCP2515_SS = 0;
   
    spi_transmit(MCP2515_CMD_READ_RX | address);
    unsigned char sidh = spi_transmit(0xff);
    unsigned char sidl = spi_transmit(0xff);

    if (sidl & 0x08) {
        // extended
        p_canmsg->flags.extended = 1;
        p_canmsg->id =  (unsigned long) sidh << 21;
        p_canmsg->id |= (unsigned long)(sidl & 0xe0) << 13;
        p_canmsg->id |= (unsigned long)(sidl & 0x03) << 16;
        p_canmsg->id |= (unsigned long) spi_transmit(0xff) << 8;
        p_canmsg->id |= (unsigned long) spi_transmit(0xff);
        unsigned char dlc = spi_transmit(0xff);
        p_canmsg->dlc = dlc & 0x0f;
        p_canmsg->flags.rtr = (dlc >> 6) & 0x01;
    } else {
        // standard
        p_canmsg->flags.extended = 0;
        p_canmsg->flags.rtr = (sidl >> 4) & 0x01;
	p_canmsg->id =  (unsigned long) sidh << 3;
        p_canmsg->id |= (unsigned long) sidl >> 5;
        spi_transmit(0xff);
        spi_transmit(0xff);
        p_canmsg->dlc = spi_transmit(0xff) & 0x0f;
    }

    // get data
    if (!p_canmsg->flags.rtr) {
        unsigned char i;
        unsigned char length = p_canmsg->dlc;
        if (length > 8) length = 8;
        for (i = 0; i < length; i++) {
            p_canmsg->data[i] = spi_transmit(0xff);
        }
    }

    // release SS, end of read buffer (clears RXnIF flag)
    MCP2515_SS = 1;

    if (current_rx_buffer == 1) {
        current_rx_buffer = 0;
    } else if (mcp2515_getPinstateRX1BF()) {
        // message in RXB1
        current_rx_buffer = 1;
    }

    return 1;
}