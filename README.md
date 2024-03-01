# wmo_oc_kmod

Kernel module for overclocking generic controllers on XHCI controllers, based on YeaSeb's driver for the WMO1.1a mouse.

The default overclock is from 125 Hz to 1000 Hz. The GC should be able to overclock to 1000hz with no issues but your mileage may vary, if your particular controller can't handle it try going for 500hz.

You should have already tried the official instructions on ArchWiki before needing this, if you have a native EHCI controller then you may be better off using that.

## TODO: 

Decide if trying to read usbhid mousepoll option or making another one (see Discussions)

Decide if making the VID:PID variables a list or array, receive new ones as a kernel parameter or try targetting all mouses asking usbhid somehow. (see discussions)

## Building

Use `make` to build wmo_oc.ko and `sudo insmod xbox_oc.ko` to load the module into the running kernel.


If you want to unload the module (revert the increased polling rate) use `sudo rmmod xbox_oc.ko`. You can also use `make clean` to clean up any files created by `make`.

If you get an error saying "building multiple external modules is not supported" it's because you have a space somewhere in the path to the gcadapter-oc-kmod directory.

GNU Make can't handle spaces in filenames so move the directory to a path without spaces (example: `/home/falco/My Games/xbox-oc-kmod` -> `/home/falco/xbox-oc-kmod`).

## Installing

A PKGBUILD is available for Arch Linux in `packaging/`. This package uses DKMS to install and auto-update the module when the kernel is updated. A configuration file is added to load the module automatically on boot.

Prepackaged versions can be found under "Releases".

For other distros copying the module to an appropriate directory under `/usr/lib/modules` and creating a file called `/usr/lib/modules-load.d/xbox-oc.conf` with the contents `xbox-oc` should be enough to load the module automatically. You'll need to rebuild the module and copy every time you upgrade your kernel so I don't recommend it!

## Changing the polling rate

Polling rate is set according to the `bInterval` value in the USB endpoint descriptor. The value sets the polling rate in milliseconds, for example: an interval value of 4 equals 250 Hz.

You can change the rate by using the kernel parameter `xbox_oc.rate=n` (if installed), passing the rate to `insmod xbox_oc.ko rate=n` or going into `/sys/module/xbox_oc/parameters` and using `echo n > rate` to change the value

## Using a different mouse

On a terminal, use the lsusb command
It should give you an output like this 

```
Bus 004 Device 002: ID 05e3:0626 Genesys Logic, Inc. Hub

Bus 004 Device 001: ID 1d6b:0003 Linux Foundation 3.0 root hub

Bus 003 Device 005: ID 1b1c:0c0b Corsair Lighting Node Pro

Bus 003 Device 003: ID 258a:0016 BY Tech Usb Gaming Keyboard

Bus 003 Device 002: ID 05e3:0610 Genesys Logic, Inc. Hub

Bus 003 Device 001: ID 1d6b:0002 Linux Foundation 2.0 root hub

Bus 002 Device 001: ID 1d6b:0003 Linux Foundation 3.0 root hub

Bus 001 Device 002: ID 045e:0040 Microsoft Corp. Wheel Mouse Optical

Bus 001 Device 001: ID 1d6b:0002 Linux Foundation 3.0 root hub 
```

Bus 001 Device 002: ID 045e:0040 Microsoft Corp. Wheel Mouse Optical
```

The VID:PID values are 045e:0040. 045e is the Vendor ID and 0040 is the Product ID. If your mouse has different values this won't work unless you edit the code.

For this, you can go to wmo_oc.c and edit lines 6 and 7 to match your VID:PID values, after that you can build like normal.
