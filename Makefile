INSTALL = /usr/bin/install -c
INSTALLDATA = /usr/bin/install -c -m 644
PROGNAME = footswitch
CFLAGS = -Wall
LDFLAGS = -lusb-1.0
# The MAC_DIR defaults to '/opt/local' which is what macports
# follows. If instead you use homebrew or somesuch or just have a different
# path where libusb is installed then prefix that in the `make` command.
# e.g. `MAC_DIR=/usr/local make install-mac`
MAC_DIR ?= /opt/local
# The MAC_INSTALL_DIR defaults to `$(MAC_DIR)/bin`
# if you want to specify a different path then you can do so as follows:
# `MAC_INSTALL_DIR=/usr/local/bin make install-mac`
MAC_INSTALL_DIR ?= $(MAC_DIR)/bin

all: $(PROGNAME)

$(PROGNAME): $(PROGNAME).c common.h common.c debug.h debug.c
	$(CC) $(PROGNAME).c common.c debug.c -o $(PROGNAME) $(CFLAGS) $(LDFLAGS)

install: all
	$(INSTALL) $(PROGNAME) /usr/bin
	$(INSTALLDATA) 19-footswitch.rules /etc/udev/rules.d

mac: $(PROGNAME).c common.h common.c debug.h debug.c
	$(CC) -I$(MAC_DIR)/include -L$(MAC_DIR)/lib $(PROGNAME).c common.c debug.c -o $(PROGNAME) $(CFLAGS) $(LDFLAGS)

install-mac: mac
	$(INSTALL) $(PROGNAME) $(MAC_INSTALL_DIR)

uninstall-mac:
	rm -f $(MAC_INSTALL_DIR)/$(PROGNAME)

clean:
	rm -f $(PROGNAME) *.o

.PHONY: install
	install-mac
	clean
