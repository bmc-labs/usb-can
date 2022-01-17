/********************************************************************
 File: usb_cdc.c

 Description:
 This file contains the USB CDC functions.

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


#define USTAT_EP0_OUT 0x00
#define USTAT_EP0_IN 0x04
#define USTAT_EP2_IN 0x14
#define USTAT_EP3_OUT 0x18
#define USTAT_EP1_IN 0x0C

#define USB_PID_SETUP 0xD

#define REQUEST_GET_STATUS 0x00
#define REQUEST_CLEAR_FEATURE 0x01
#define REQUEST_SET_FEATURE 0x03
#define REQUEST_SET_ADDRESS 0x05
#define REQUEST_GET_DESCRIPTOR 0x06
#define REQUEST_GET_CONFIGURATION 0x08
#define REQUEST_SET_CONFIGURATION 0x09
#define REQUEST_GET_INTERFACE 0x0A
#define REQUEST_SET_INTERFACE 0x11
#define REQUEST_SYNCH_FRAME 0x12

#define REQUEST_SEND_ENCAPSULATED_COMMAND 0x00
#define REQUEST_GET_ENCAPSULATED_RESPONSE 0x01
#define REQUEST_SET_LINE_CODING           0x20
#define REQUEST_GET_LINE_CODING           0x21
#define REQUEST_SET_CONTROL_LINE_STATE    0x22

#define DESCR_DEVICE 0x01
#define DESCR_CONFIG 0x02
#define DESCR_STRING 0x03
#define DESCR_INTERFACE 0x04
#define DESCR_ENDPOINT 0x05

#define USB_DEV_DESC_SERIALNUMBER_OFFSET 16
#define USB_STRING_SERIALNUMBER_INDEX 3
#define USB_STRING_SERIALNUMBER_SIZE 18

#define EP_BUFFERSIZE_BULK 0x40

typedef struct
{
    unsigned char stat;
    unsigned char cnt;
    unsigned char adrl;
    unsigned char adrh;
} BDT;     

#define EPBD_NROF 12

// datasheet table 22-2 page 264 Mode 3
#define EPBD_EP0_OUT 0
#define EPBD_EP0_IN 1
#define EPBD_EP1_IN_EVEN 4
#define EPBD_EP1_IN_ODD 5
#define EPBD_EP2_IN_EVEN 8
#define EPBD_EP2_IN_ODD 9
#define EPBD_EP3_OUT_EVEN 10
#define EPBD_EP3_OUT_ODD 11


#define EVEN 0
#define ODD 1



/* USB request type values */
#define USBRQ_TYPE_MASK         0x60
#define USBRQ_TYPE_STANDARD     (0<<5)
#define USBRQ_TYPE_CLASS        (1<<5)
#define USBRQ_TYPE_VENDOR       (2<<5)

#define USBRQ_RECIPIENT_MASK    0x1f
#define USBRQ_RECIPIENT_DEVICE  0x00
#define USBRQ_RECIPIENT_INTERFACE 0x01
#define USBRQ_RECIPIENT_ENDPOINT 0x02
#define USBRQ_RECIPIENT_OTHER   0x03

const unsigned char usb_dev_desc[] = {
	18,
	0x01,
	0x00, 0x02,	
	0x02, // Class code
	0x00,	
	0x00,	
	0x08, // max packet size
	0xd8, 0x04, // vendor
	0x0a, 0x00, // product
	0x00, 0x01, // device release
	0x01, // manuf string
	0x02, // product string
	0x00, // serial number string (if number available, index 3 is set on the fly)
	0x01
};

const unsigned char usb_config_desc[] = {
/*Configuation Descriptor*/
        0x09,   /* bLength: Configuation Descriptor size */
        DESCR_CONFIG,      /* bDescriptorType: Configuration */
        9+9+5+5+4+5+7+9+7+7,       /* wTotalLength:no of returned bytes */
        0x00,
        0x02,   /* bNumInterfaces: 2 interface */
        0x01,   /* bConfigurationValue: Configuration value */
        0x00,   /* iConfiguration: Index of string descriptor describing the configuration */
        0x80,   /* bmAttributes: bus powered */
        50,     /* MaxPower 100 mA */
/*Interface Descriptor*/
        0x09,   /* bLength: Interface Descriptor size */
        DESCR_INTERFACE,  /* bDescriptorType: Interface */
                        /* Interface descriptor type */
        0x00,   /* bInterfaceNumber: Number of Interface */
        0x00,   /* bAlternateSetting: Alternate setting */
        0x01,   /* bNumEndpoints: One endpoints used */
        0x02,   /* bInterfaceClass: Communication Interface Class */
        0x02,   /* bInterfaceSubClass: Abstract Control Model */
        0x01,   /* bInterfaceProtocol: Common AT commands */
        0x00,   /* iInterface: */
/*Header Functional Descriptor*/
        0x05,   /* bLength: Endpoint Descriptor size */
        0x24,   /* bDescriptorType: CS_INTERFACE */
        0x00,   /* bDescriptorSubtype: Header Func Desc */
        0x10,   /* bcdCDC: spec release number */
        0x01,

/*ACM Functional Descriptor*/
        0x04,   /* bFunctionLength */
        0x24,   /* bDescriptorType: CS_INTERFACE */
        0x02,   /* bDescriptorSubtype: Abstract Control Management desc */
        0x02,   /* bmCapabilities */ 

/*Union Functional Descriptor*/
        0x05,   /* bFunctionLength */
        0x24,   /* bDescriptorType: CS_INTERFACE */
        0x06,   /* bDescriptorSubtype: Union func desc */
        0x00,   /* bMasterInterface: Communication class interface */
        0x01,   /* bSlaveInterface0: Data Class Interface */

/*Call Managment Functional Descriptor*/
        0x05,   /* bFunctionLength */
        0x24,   /* bDescriptorType: CS_INTERFACE */
        0x01,   /* bDescriptorSubtype: Call Management Func Desc */
        0x00,   /* bmCapabilities: D0+D1 */
        0x01,   /* bDataInterface: 1 */


/*Endpoint 2 Descriptor*/
        0x07,   /* bLength: Endpoint Descriptor size */
        DESCR_ENDPOINT,   /* bDescriptorType: Endpoint */
        0x82,   /* bEndpointAddress: (IN2) */
        0x03,   /* bmAttributes: Interrupt */
        0x08,      /* wMaxPacketSize: ???????????????????? */
        0x00,
        0x02,   /* bInterval: */
/*Data class interface descriptor*/
        0x09,   /* bLength: Endpoint Descriptor size */
        DESCR_INTERFACE,  /* bDescriptorType: */
        0x01,   /* bInterfaceNumber: Number of Interface */
        0x00,   /* bAlternateSetting: Alternate setting */
        0x02,   /* bNumEndpoints: Two endpoints used */
        0x0A,   /* bInterfaceClass: CDC */
        0x00,   /* bInterfaceSubClass: */
        0x00,   /* bInterfaceProtocol: */
        0x00,   /* iInterface: */
/*Endpoint 3 Descriptor*/
        0x07,   /* bLength: Endpoint Descriptor size */
        DESCR_ENDPOINT,   /* bDescriptorType: Endpoint */
        0x03,   /* bEndpointAddress: (OUT3) */
        0x02,   /* bmAttributes: Bulk */
        0x08,             /* wMaxPacketSize: */
        0x00,
        0x00,   /* bInterval: ignore for Bulk transfer */
/*Endpoint 1 Descriptor*/
        0x07,   /* bLength: Endpoint Descriptor size */
        DESCR_ENDPOINT,   /* bDescriptorType: Endpoint */
        0x81,   /* bEndpointAddress: (IN1) */
        0x02,   /* bmAttributes: Bulk */
        EP_BUFFERSIZE_BULK,  /* wMaxPacketSize: */
        0x00,
        0x00    /* bInterval: ignore for Bulk transfer */
};

const unsigned char usb_string_0[] = {
    4, // length
    0x03, // descriptor type
    0x09, 0x04, // english
};

const unsigned char usb_string_manuf[] = {
	0x36,	
	0x03,	// type
	'M', 0x00, 
	'i', 0x00, 
	'c', 0x00, 
	'r', 0x00, 
	'o', 0x00, 
	'c', 0x00, 
	'h', 0x00, 
	'i', 0x00, 
	'p', 0x00, 
	' ', 0x00,
	'T', 0x00, 
	'e', 0x00, 
	'c', 0x00, 
	'h', 0x00, 
	'n', 0x00, 
	'o', 0x00, 
	'l', 0x00, 
	'o', 0x00, 
	'g', 0x00, 
	'y', 0x00, 
	',', 0x00, 
	' ', 0x00,
	'I', 0x00, 
	'n', 0x00, 
	'c', 0x00, 
	'.', 0x00
};

const unsigned char usb_string_product[] = {
    14,
    0x03, // type
    'U', 0x00,
    'S', 0x00,
    'B', 0x00,
    't', 0x00,
    'i', 0x00,
    'n', 0x00
};


#define EP_BUFFERSIZE 0x08

volatile BDT epbd[EPBD_NROF] @ 0x200;
// 12 BDTs in use -> we can set buffers starting at 0x230
volatile unsigned char ep0out_buffer[EP_BUFFERSIZE] @ 0x230;
volatile unsigned char ep0in_buffer[EP_BUFFERSIZE] @ 0x238;
volatile unsigned char ep2in_buffer[2][EP_BUFFERSIZE] @ 0x240;
volatile unsigned char ep3out_buffer[2][EP_BUFFERSIZE] @ 0x250;
volatile unsigned char ep1in_buffer[2][EP_BUFFERSIZE_BULK] @ 0x260;


unsigned configured = 0;
unsigned char usb_config = 0;
unsigned char usb_setaddress = 0;
unsigned short usb_sendleft = 0;
const unsigned char * usb_sendbuffer;

unsigned char txbuffer_writepos = 0;
unsigned char usb_getchpos = 0;
unsigned char linecoding[7];
unsigned char dolinecoding = 0;

unsigned char current_ep1_buffer = EVEN;
unsigned char current_ep3_buffer = EVEN;
unsigned char nosend_counter = 0;
unsigned char usb_ep0status[2] = {0, 0};

/**
 * Determine if usb is enumerated and interface is configured
 * @return 0 if device is not configured
 */
unsigned char usb_isConfigured() {
    return configured;
}

/**
 * Determine if serial number is available
 * @return 0 if no number available
 */
unsigned char usb_serialNumberAvailable() {
    return ((usb_string_serial[0] == USB_STRING_SERIALNUMBER_SIZE) && (usb_string_serial[1] == 0x03));
}

/**
 * Process pending send activity
 */
void usb_sendProcess() {
    if (usb_sendleft == 0) return;
    
    unsigned short length = usb_sendleft; 
    if (length > EP_BUFFERSIZE)
        length = EP_BUFFERSIZE;

    unsigned char i;
    for (i = 0; i < length; i++) {

        if ((usb_sendbuffer == usb_dev_desc + USB_DEV_DESC_SERIALNUMBER_OFFSET) && usb_serialNumberAvailable()) ep0in_buffer[i] = USB_STRING_SERIALNUMBER_INDEX;
        else ep0in_buffer[i] = *usb_sendbuffer;

        usb_sendbuffer ++;
        usb_sendleft--;
    }

    epbd[EPBD_EP0_IN].cnt = length;
    if (epbd[EPBD_EP0_IN].stat & 0x40)
        epbd[EPBD_EP0_IN].stat = 0x88;
    else
        epbd[EPBD_EP0_IN].stat = 0xC8;    
}

/**
 * Load given descriptor buffer into send buffer
 *
 * @param descbuffer Pointer to descriptor
 * @param size Size of descriptor
 * @param length Count of bytes to send
 * @return 0 on error, 1 on success
 */
unsigned char usb_loadDescriptor(const unsigned char * descbuffer, unsigned short size, unsigned short length) {

    if (length > size)
        length = size;

    usb_sendleft = length;
    usb_sendbuffer = descbuffer;

    usb_sendProcess();
    return length;
}

/**
 * Handle descriptor requests
 *
 * @param type Type of descriptor
 * @param index Index of descriptor
 * @param length Requested length
 * @return 0 on error, 1 on success
 */
unsigned char usb_handleDescriptorRequest(unsigned char type, unsigned char index, unsigned short length) {

    switch (type) {
        case DESCR_DEVICE:
            return usb_loadDescriptor(usb_dev_desc, sizeof(usb_dev_desc), length);
        case DESCR_CONFIG:
            return usb_loadDescriptor(usb_config_desc, sizeof(usb_config_desc), length);
        case DESCR_STRING:
            switch (index) {
                case 0: return usb_loadDescriptor(usb_string_0, sizeof(usb_string_0), length);
                case 1: return usb_loadDescriptor(usb_string_manuf, sizeof(usb_string_manuf), length);            
                case 2: return usb_loadDescriptor(usb_string_product, sizeof(usb_string_product), length);            
                case 3: return usb_loadDescriptor(usb_string_serial, USB_STRING_SERIALNUMBER_SIZE, length);            
            }
    }

    return 0;
}

/**
 * Determine if endpoint 1 is ready to accept characters
 * 
 * @retval 0 Not ready
 * @retval 1 Ready
 */
unsigned char usb_ep1_ready() {
    return (epbd[EPBD_EP1_IN_EVEN + current_ep1_buffer].stat & 0x80) == 0;
}

/**
 * Flush endpoint 1. Send out pending characters
 */
void usb_ep1_flush() {
    if (!configured) return;
    if (txbuffer_writepos == 0) return;
    if (epbd[EPBD_EP1_IN_EVEN + current_ep1_buffer].stat & 0x80) return;

    epbd[EPBD_EP1_IN_EVEN + current_ep1_buffer].cnt = txbuffer_writepos;
    txbuffer_writepos = 0;
    
    if (current_ep1_buffer == EVEN) {
        epbd[EPBD_EP1_IN_EVEN].stat = 0xC8;
        current_ep1_buffer = ODD;
    } else {
        epbd[EPBD_EP1_IN_ODD].stat = 0x88;
        current_ep1_buffer = EVEN;
    }    
}

/**
 * Put given character into send buffer
 *
 * @param ch Character to send
 */
void usb_putch(unsigned char ch) {

    if (epbd[EPBD_EP1_IN_EVEN + current_ep1_buffer].stat & 0x80) {
        // overflow! TODO: signal overflow (->errorflags?)        
        return;
    }
    
    ep1in_buffer[current_ep1_buffer][txbuffer_writepos] = ch;
    
    nosend_counter = 0;
    
    txbuffer_writepos++;
    if (txbuffer_writepos == EP_BUFFERSIZE_BULK) {
        usb_ep1_flush();
    }
}

/**
 * Put given nullterminated string into send buffer
 *
 * @param s String to send
 */
void usb_putstr(char * s) {
   while (*s) {
     usb_putch((unsigned char) *s);
     s++;
   }
}

/**
 * Determine if there are received characters
 *
 * @retval 1 if there are characters in the receive buffer
 * @retval 0 receive buffer empty
 */
unsigned char usb_chReceived() {
    return (epbd[EPBD_EP3_OUT_EVEN + current_ep3_buffer].stat & 0x80) == 0;
}

/**
 * Read character from receive buffer
 *
 * @return Character read from receive buffer
 */
unsigned char usb_getch() {
    while (!usb_chReceived) {}

    unsigned char ch = ep3out_buffer[current_ep3_buffer][usb_getchpos];
    usb_getchpos++;
    if (usb_getchpos == epbd[EPBD_EP3_OUT_EVEN + current_ep3_buffer].cnt) {
        epbd[EPBD_EP3_OUT_EVEN + current_ep3_buffer].cnt = EP_BUFFERSIZE;
        epbd[EPBD_EP3_OUT_EVEN + current_ep3_buffer].stat = 0x80;
        usb_getchpos = 0;
        
        current_ep3_buffer = !current_ep3_buffer;
    }
    return ch;
}

/**
 * Initialize USB module
 */
void usb_init() {
    
    epbd[EPBD_EP0_OUT].stat = 0x80;
    epbd[EPBD_EP0_OUT].cnt = EP_BUFFERSIZE;
    epbd[EPBD_EP0_OUT].adrl = 0x30;
    epbd[EPBD_EP0_OUT].adrh = 0x02;

    epbd[EPBD_EP0_IN].stat = 0;
    epbd[EPBD_EP0_IN].cnt = EP_BUFFERSIZE;
    epbd[EPBD_EP0_IN].adrl = 0x38;
    epbd[EPBD_EP0_IN].adrh = 0x02;

    
    epbd[EPBD_EP1_IN_EVEN].stat = 0x00;
    epbd[EPBD_EP1_IN_EVEN].cnt = EP_BUFFERSIZE_BULK;
    epbd[EPBD_EP1_IN_EVEN].adrl = 0x60;
    epbd[EPBD_EP1_IN_EVEN].adrh = 0x02;

    epbd[EPBD_EP1_IN_ODD].stat = 0x40;
    epbd[EPBD_EP1_IN_ODD].cnt = EP_BUFFERSIZE_BULK;
    epbd[EPBD_EP1_IN_ODD].adrl = 0xA0;
    epbd[EPBD_EP1_IN_ODD].adrh = 0x02;

    
    epbd[EPBD_EP2_IN_EVEN].stat = 0x00;
    epbd[EPBD_EP2_IN_EVEN].cnt = EP_BUFFERSIZE;
    epbd[EPBD_EP2_IN_EVEN].adrl = 0x40;
    epbd[EPBD_EP2_IN_EVEN].adrh = 0x02;
    
    epbd[EPBD_EP2_IN_ODD].stat = 0x40;
    epbd[EPBD_EP2_IN_ODD].cnt = EP_BUFFERSIZE;
    epbd[EPBD_EP2_IN_ODD].adrl = 0x48;
    epbd[EPBD_EP2_IN_ODD].adrh = 0x02;

    
    epbd[EPBD_EP3_OUT_EVEN].stat = 0x80;
    epbd[EPBD_EP3_OUT_EVEN].cnt = EP_BUFFERSIZE;
    epbd[EPBD_EP3_OUT_EVEN].adrl = 0x50;
    epbd[EPBD_EP3_OUT_EVEN].adrh = 0x02;

    epbd[EPBD_EP3_OUT_ODD].stat = 0x80;
    epbd[EPBD_EP3_OUT_ODD].cnt = EP_BUFFERSIZE;
    epbd[EPBD_EP3_OUT_ODD].adrl = 0x58;
    epbd[EPBD_EP3_OUT_ODD].adrh = 0x02;
    
    
    UEP0 = 0x16;
    UEP1 = 0x1A;
    UEP2 = 0x1A;
    UEP3 = 0x1C;

    UCFG = 0x17;
    UCON = 0x08;
}


/**
 * Do USB processing
 */
void usb_process() {
       
    // auto flush after some idle time
    if (nosend_counter++ > 200) {
        usb_ep1_flush();
    }

    if (UIRbits.TRNIF) {
        // complete interrupt

            if (USTAT == USTAT_EP0_OUT) {

                    // out/setup

                    if (((epbd[EPBD_EP0_OUT].stat >> 2) & 0x0F) == USB_PID_SETUP) {
                        // setup token

                        epbd[EPBD_EP0_IN].stat = 0;
                        epbd[EPBD_EP0_IN].stat = 0;

			if ((ep0out_buffer[0] & USBRQ_TYPE_MASK) == USBRQ_TYPE_STANDARD) {

		                switch (ep0out_buffer[1]) {
		                    case REQUEST_GET_DESCRIPTOR:
		                        if (!usb_handleDescriptorRequest(ep0out_buffer[3], ep0out_buffer[2] , (ep0out_buffer[7] << 8) | ep0out_buffer[6])) {
                                            epbd[EPBD_EP0_IN].cnt = 0;
		                            epbd[EPBD_EP0_IN].stat = 0xCC; // Stall
                                        }
		                        break;
                                
		                    case REQUEST_SET_ADDRESS:

		                        usb_setaddress = ep0out_buffer[2];

		                        epbd[EPBD_EP0_IN].cnt = 0;
		                        epbd[EPBD_EP0_IN].stat = 0xC8;
		                        break;
                                
		                    case REQUEST_SET_CONFIGURATION:

		                        usb_config = ep0out_buffer[2];
                                configured = 1;
		                        epbd[EPBD_EP0_IN].cnt = 0;
		                        epbd[EPBD_EP0_IN].stat = 0xC8;
		                        break;

		                    case REQUEST_GET_CONFIGURATION:

		                        ep0in_buffer[0] = usb_config;
		                        epbd[EPBD_EP0_IN].cnt = 1;                         
                                epbd[EPBD_EP0_IN].stat = 0xC8;    
		                        break;

		                    case REQUEST_GET_INTERFACE:

		                        ep0in_buffer[0] = 0;
		                        epbd[EPBD_EP0_IN].cnt = 1;                         
                                epbd[EPBD_EP0_IN].stat = 0xC8;    
		                        break;
                                
                            case REQUEST_GET_STATUS:
                                if ((ep0out_buffer[0] & USBRQ_RECIPIENT_MASK) == USBRQ_RECIPIENT_ENDPOINT) {
                                    ep0in_buffer[0] = usb_ep0status[0];
                                    ep0in_buffer[1] = usb_ep0status[1];
                                } else {
                                    ep0in_buffer[0] = 0;
                                    ep0in_buffer[1] = 0;
                                }
                                epbd[EPBD_EP0_IN].cnt = 2;
		                        epbd[EPBD_EP0_IN].stat = 0xC8;
                                break;
                                
                            case REQUEST_SET_FEATURE:
                                if ((ep0out_buffer[0] & USBRQ_RECIPIENT_MASK) == USBRQ_RECIPIENT_ENDPOINT) {
                                    if (ep0out_buffer[2] == 0x00) // HALT
                                        usb_ep0status[0] = 1;
                                }
                                epbd[EPBD_EP0_IN].cnt = 0;
		                        epbd[EPBD_EP0_IN].stat = 0xC8;
		                        break;                                
                                
                            case REQUEST_CLEAR_FEATURE:
                                if ((ep0out_buffer[0] & USBRQ_RECIPIENT_MASK) == USBRQ_RECIPIENT_ENDPOINT) {
                                    usb_ep0status[0] = 0;                                    
                                }
                                epbd[EPBD_EP0_IN].cnt = 0;
		                        epbd[EPBD_EP0_IN].stat = 0xC8;
		                        break;                                
                                
                            case REQUEST_SYNCH_FRAME:
                                ep0in_buffer[0] = 0;
                                ep0in_buffer[1] = 0;
		                        epbd[EPBD_EP0_IN].cnt = 2;
		                        epbd[EPBD_EP0_IN].stat = 0xC8;
                                break;

                            case REQUEST_SET_INTERFACE:
                                epbd[EPBD_EP0_IN].cnt = 0;
		                        epbd[EPBD_EP0_IN].stat = 0xC8;
		                        break;
		                    default:		                                       
		                        epbd[EPBD_EP0_IN].cnt = 0;
		                        epbd[EPBD_EP0_IN].stat = 0xCC; // stall
		                        break;
		                        
		                }
			} else if ((ep0out_buffer[0] & USBRQ_TYPE_MASK) == USBRQ_TYPE_CLASS) {
                                switch (ep0out_buffer[1]) {
		                    
                                    case REQUEST_GET_ENCAPSULATED_RESPONSE:
                                        {unsigned char i; for (i=0; i<8; i++) {ep0in_buffer[i] = 0;}};
		                        epbd[EPBD_EP0_IN].cnt = 8;
		                        epbd[EPBD_EP0_IN].stat = 0xC8;
                                        break;                          

				    case REQUEST_SET_LINE_CODING:
                                        dolinecoding = 1;
                                        epbd[EPBD_EP0_IN].cnt = 0;
		                        epbd[EPBD_EP0_IN].stat = 0xC8;
		                        break;

                                    case REQUEST_GET_LINE_CODING:
                                        {
                                            unsigned char i;
                                            for (i = 0; i < sizeof(linecoding); i++) {
                                                ep0in_buffer[i] = linecoding[i];
                                            }
                                        }
                                        epbd[EPBD_EP0_IN].cnt = 7;
		                        epbd[EPBD_EP0_IN].stat = 0xC8;
		                        break;

                                    case REQUEST_SET_CONTROL_LINE_STATE:
                                    case REQUEST_SEND_ENCAPSULATED_COMMAND:
                                        epbd[EPBD_EP0_IN].cnt = 0;
		                        epbd[EPBD_EP0_IN].stat = 0xC8;
		                        break;
		                    default:
		                        epbd[EPBD_EP0_IN].cnt = 0;
		                        epbd[EPBD_EP0_IN].stat = 0xCC; // Stall
		                        break;
                                }
                       }


                    } else {
                          // data stage

                          if (dolinecoding) {
                              unsigned char i;
                              for (i = 0; i < sizeof(linecoding); i++) {
                                  linecoding[i] = ep0out_buffer[i];
                              }
                              dolinecoding = 0;
                          }
                    }

                    epbd[EPBD_EP0_OUT].cnt = EP_BUFFERSIZE;
                    epbd[EPBD_EP0_OUT].stat = 0x80;

            } else if (USTAT == USTAT_EP0_IN) {

                // check if set address command is pending
                if (usb_setaddress > 0) {
                    UADDR = usb_setaddress;
                    usb_setaddress = 0;
                }
               
                usb_sendProcess();

            }

            UCONbits.PKTDIS = 0;
            UIRbits.TRNIF = 0;
        }
}

