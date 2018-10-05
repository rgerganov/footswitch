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

all: scythe.c footswitch.c common.h common.c debug.h debug.c
	$(CC) footswitch.c common.c debug.c -o footswitch $(CFLAGS) $(LDLIBS)
	$(CC) scythe.c common.c debug.c -o scythe $(CFLAGS) $(LDLIBS)

install: all
	$(INSTALL) footswitch /usr/local/bin
	$(INSTALL) scythe /usr/local/bin
ifeq ($(UNAME), Linux)
	$(INSTALLDATA) 19-footswitch.rules /etc/udev/rules.d
endif

uninstall: 
	rm -f /usr/local/bin/footswitch
	rm -f /usr/local/bin/scythe
ifeq ($(UNAME), Linux)
	rm -f /etc/udev/rules.d/19-footswitch.rules
endif

clean:
	rm -f scythe footswitch *.o

