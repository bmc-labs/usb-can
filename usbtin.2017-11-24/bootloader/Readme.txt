Bootloader for USBtin
---------------------

The bootloader for USBtin is adapted from Microchips USB HID Bootloader for
PIC18 Non-J Families (/microchip/mla/v2015_08_10/apps/usb/device/bootloaders/
firmware/pic18_non_j).

Device:
USBtin with PIC18F14K50

Changes:
- Use Jumper to enter bootloader
- Set up clock from MCP2515
- Disabled EEPROM functions to reduce code size
- Added USB serialnumber string

Licence:
The bootloader is closed source. The binary (hex) file is provided free for
personal use. For closed or commercial projects you have to contact the author
to get the permission for using this source code.

Usage:
Program the bootloader into the device/USBtin with a programmer which
supports PIC18 controllers (e.g. PicKit 3).
To enter the bootloader, set the bootloader jumper (JP1) on USBtin before
connecting it to the USB. Then you can use a loader application to update the
firmware. "mphidflash" is such an application. Example:
mphidflash -write USBtin_firmware_v1.x.hex
After flashing a new fimware, disconnect the USBtin from the USB and open the
jumper before reconnecting the device to the host pc.

History:
- 2012-02-09   v1.0    Initial version (based on MLA v2011-10-18-beta)
- 2016-04-13   v1.1    Added USB serialnumber string (based on MLA v2015_08_10)


2016-04-13 Thomas Fischl <tfischl@gmx.de>
http://www.fischl.de/usbtin/




