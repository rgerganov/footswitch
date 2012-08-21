Footswitch
----------

Command line utility for programming the [foot switch][1] sold by PCsensor (vendorId:productId = 0c45:7403). The same foot switch is used for building the popular [VIM Clutch][2].

Building
------------

The program is using the [libusb-1.0][3] library and should work on all major platforms. To build on Debian/Ubuntu:

 * `sudo apt-get install libusb-1.0-0.dev`
 * `make`

Examples
--------

Note: you may need to run `footswitch` as root on some systems

 * `footswitch -r` -- read the currently persisted function and print it on the console
 * `footswitch -s 'hello world'` -- write the string 'hello world' when pressed
 * `footswitch -S '29 C' -- write the specified "raw" string (hex numbers delimited with spaces) when pressed; in this particular example the switch will produce "Escape" followed by the letter "i". You can find the hex code for each key [here][4].
 * `footswitch -k f4 -m alt` -- produce Alt+F4 when the switch is pressed
 * `footswitch -k tab -m ctrl -m shift` -- produce Ctrl+Shift+Tab when the switch is pressed
 * `footswitch -x 10 -y -20` -- move the mouse cursor 10 pixels right and 20 pixels up
 * `footswitch -w 20` -- move the mouse wheel 20 units up

[1]: http://www.pcsensor.com/index.php?_a=viewProd&productId=2
[2]: https://github.com/alevchuk/vim-clutch
[3]: http://www.libusb.org/
[4]: http://www.freebsddiary.org/APC/usb_hid_usages.php

