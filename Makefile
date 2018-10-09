PREFIX = /usr/local
INSTALL = /usr/bin/install -c
INSTALLDATA = /usr/bin/install -c -m 644
CFLAGS = -Wall
UNAME := $(shell uname)
ifeq ($(UNAME), Darwin)
	CFLAGS += -DOSX $(shell pkg-config --cflags hidapi)
	LDLIBS = $(shell pkg-config --libs hidapi)
else
	ifeq ($(UNAME), Linux)
		CFLAGS += $(shell pkg-config --cflags hidapi-libusb)
		LDLIBS = $(shell pkg-config --libs hidapi-libusb)
	else
		LDLIBS = -lhidapi
	endif
endif

all: footswitch scythe

footswitch: footswitch.c common.c debug.c
scythe: scythe.c common.c debug.c

install: all
	$(INSTALL) footswitch $(PREFIX)/bin
	$(INSTALL) scythe $(PREFIX)/bin
ifeq ($(UNAME), Linux)
	$(INSTALLDATA) 19-footswitch.rules /etc/udev/rules.d
endif

uninstall: 
	rm -f $(PREFIX)/bin/footswitch
	rm -f $(PREFIX)/bin/scythe
ifeq ($(UNAME), Linux)
	rm -f /etc/udev/rules.d/19-footswitch.rules
endif

clean:
	rm -f scythe footswitch *.o

