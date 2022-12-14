# `usb-can`, a simple and cost effective CAN link

![bmc::labs `usb-can`](./pictures/usb-can_pcb-complete_02.jpg "bmc::labs `usb-can`")
_Rendering of `usb-can`, our version of the USBtin by Thomas Fischl_

Be it for automation, development or debugging, one often needs a way to
inspect a CAN bus. This device allows to connect to a CAN bus via a standard
USB type C cable. It is based on the [USBtin by Thomas
Fischl](https://www.fischl.de/usbtin/) - there are many great resources behind
this link, and we encourage you to buy Thomas' version from him!

All examplatory output comes from a 2013 MacBook Pro running Debian.


## Use with Linux
Start by adding yourself to the `dialout` group if you aren't already in it:

```bash
» sudo usermod -aG dialout $USER
```

You'll need [`can-utils` for Linux](https://github.com/linux-can/can-utils).
If you're on something Debian-based, you're best off installing them like so:

```bash
» sudo apt install can-utils
```

If you're on a different distro, you may find the `can-utils` in your
respective package management; if not go get them [from
GitHub](https://github.com/linux-can/can-utils). Good, now you're all set.

#### Loading kernel modules
CAN support via `socketcan` is upstream in Linux, but it is not necessarily
loaded by default. Execute load like so:

```bash
» sudo modprobe can
» sudo modprobe can-raw
» sudo modprobe slcan
```

You can use the `lsmod` utility to verify everything has worked:

```bash
» lsmod | grep can
slcan                  16384  0
can_raw                20480  0
can                    24576  1 can_raw
```

You output may look slightly different, but you should see the modules loaded.

#### Check which serial device is the CAN device
There are a number of ways to do this. First off, you may want to check that
the `usb-can` is actually recognized via `lsusb` (which you may have to
install):

```bash
» lsusb
...
Bus 002 Device 015: ID 04d8:000a Microchip Technology, Inc. CDC RS-232 Emulation Demo
...
```

If the device shows up here, you can look which serial port it is bound to:

```bash
» sudo tail /var/log/kern.log
Apr 18 14:53:17 rzrgrl kernel: [64635.125815] usb 2-2: new full-speed USB device number 15 using xhci_hcd
Apr 18 14:53:17 rzrgrl kernel: [64635.276572] usb 2-2: New USB device found, idVendor=04d8, idProduct=000a, bcdDevice= 1.00
Apr 18 14:53:17 rzrgrl kernel: [64635.276580] usb 2-2: New USB device strings: Mfr=1, Product=2, SerialNumber=0
Apr 18 14:53:17 rzrgrl kernel: [64635.276583] usb 2-2: Product: USBtin
Apr 18 14:53:17 rzrgrl kernel: [64635.276585] usb 2-2: Manufacturer: Microchip Technology, Inc.
Apr 18 14:53:17 rzrgrl kernel: [64635.277670] cdc_acm 2-2:1.0: ttyACM0: USB ACM device
```

In this case, it is `ttyACM0`, which should be similar for you.

#### Attach the device to a socket-capable interface
The following commands come from `can-utils`, so if you haven't installed those
so far, do that now.

Next, you should select the bus speed of the CAN you're trying to connect to.
The available speeds are:

| Baudrate (kBaud) | Number encoding | Command paramete |
| :---: | :---: | :---: |
| 1.000 | 8 | `-s8` |
| 800 | 7 | `-s7` |
| 500 | 6 | `-s6` |
| 250 | 5 | `-s5` |
| 125 | 4 | `-s4` |
| 100 | 3 | `-s3` |
| 50 | 2 | `-s2` |
| 20 | 1 | `-s1` |
| 10 | 0 | `-s0` |

Setting up a 500 kBaud CAN interface at serial port `ttyACM0` is done like so:

```bash
» sudo slcan_attach -f -s6 -o /dev/ttyACM0
attached tty /dev/ttyACM0 to netdevice slcan0
» sudo slcand ttyACM0 slcan0
» sudo ip link set slcan0 up
```

**Congratulations, you've done it!**

Now you can send and receive CAN messages on the socket interface `slcan0`. For
example, to dump all incoming messages to the terminal:

```bash
» candump slcan0
```

The `candump` utility also allows you to filter for CAN IDs, set up the byte
order, and a number of other useful things it will tell you about if you as it
nicely (`candump -h`).

Sending a frame to CAN is similarly easy:

```bash
» cansend slcan0 7ff#deadbeef
```

The `cansend` utility is also very vocal about its capabilities if asked.

Further useful components of `can-utils`:

1. the `canplayer` utility, which lets you play CAN messages from a log file
    that looks like this:

    ```
    (0.100) slcan0 5D1#0000
    (0.200) slcan0 271#0100
    (0.300) slcan0 289#72027000
    (0.400) slcan0 401#081100000000
    ```

    The first column is a timestamp allowing for time controlled dumping of the
    frames, the second and third are (hopefully) self-explanatory by now.
2. the `canbusload` utility for analyzing the load on the bus.

Both utilities will also happily tell you about themselves with `-h`.

#### Usage example
Last but not least, here is a snippet for sending the current time in the
format `{hour}{minute}` to the CAN on ID `0x7ff`, i.e. the last and therefore
least priority ID, every 5 seconds:

```bash
while true
do
  cansend slcan0 7ff#$(printf '%02x%02x' $(date +"%I") $(date +"%M"))
  sleep 5
done
```


## Use with Windows, macOS

Please find hints for Windows on [Thomas Fischl's
website](https://www.fischl.de/usbtin/).

For macOS, there is code floating around the web for
[`socketcan`](https://github.com/duraki/socketcanx/) and for
[`can-utils`](https://github.com/carloop/can-utils-osx), and once you have
those working the above guide for Linux should mostly apply. No idea if it
works, though - we don't do macOS right now.

---
_The contents of this repository which are our original work are released into
the public domain unter The Unlicense (see [LICENSE](./LICENSE)). We build
heavily on the work of others here and recommend you pay them tribute first if
you like the material you've found here._

## Create your own

### Bill of Materials
- 1x [usb-can_housing-lower](https://gitlab.bmc-labs.com/bmc-board/usb-can/-/blob/trunk/design-files/usb-can_housing-lower.step) (3d print)
- 1x [usb-can_housing-lower-clean](https://gitlab.bmc-labs.com/bmc-board/usb-can/-/blob/trunk/design-files/usb-can_housing-upper-clean.step) or [-icon](https://gitlab.bmc-labs.com/bmc-board/usb-can/-/blob/trunk/design-files/usb-can_housing-upper-icon.step) version (3d print)
- 4x [Socket screw M3x8mm](https://uk.rs-online.com/web/p/socket-screws/1871213)
- 4x [Thread insert M3x4.78mm](https://uk.rs-online.com/web/p/threaded-inserts/0278534/?relevancy-data=7365617263685F636173636164655F6F726465723D31267365617263685F696E746572666163655F6E616D653D4931384E525353746F636B4E756D626572267365617263685F6C616E67756167655F757365643D656E267365617263685F6D617463685F6D6F64653D6D61746368616C6C267365617263685F7061747465726E5F6D6174636865643D5E2828282872737C5253295B205D3F293F285C647B337D5B5C2D5C735D3F5C647B332C347D5B705061415D3F29297C283235285C647B387D7C5C647B317D5C2D5C647B377D29292924267365617263685F7061747465726E5F6F726465723D31267365617263685F73745F6E6F726D616C697365643D59267365617263685F726573706F6E73655F616374696F6E3D267365617263685F747970653D52535F53544F434B5F4E554D424552267365617263685F77696C645F63617264696E675F6D6F64653D4E4F4E45267365617263685F6B6579776F72643D3237382D353334267365617263685F6B6579776F72645F6170703D30323738353334267365617263685F636F6E6669673D3026&searchHistory=%7B%22enabled%22%3Atrue%7D)
- 1x usb-can PCB board
