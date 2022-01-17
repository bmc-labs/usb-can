MAIN           = main
SRC            = main.c usb_cdc.c mcp2515.c clock.c frontend.c
CC             = /opt/microchip/xc8/v1.44/bin/xc8
BOOTLOADER     = mphidflash

all: $(MAIN).hex

$(MAIN).hex: $(SRC)
	$(CC) $(SRC) --chip=18F14K50 --CODEOFFSET=1000h --ROM=default,-0-fff --MODE=pro --OPT=+speed

flash: 
	$(BOOTLOADER) -write main.hex -reset

clean:
	rm -f $(MAIN).hex funclist $(MAIN).cof $(MAIN).hxl $(MAIN).p1 $(MAIN).sdb startup.* $(MAIN).lst $(MAIN).pre $(MAIN).sym

