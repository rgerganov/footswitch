INSTALL = /usr/bin/install -c
INSTALLDATA = /usr/bin/install -c -m 644
CFLAGS = -Wall
UNAME := $(shell uname)
ifeq ($(UNAME), Darwin)
	CFLAGS += -DOSX $(shell pkg-config --cflags hidapi)
	LDFLAGS = $(shell pkg-config --libs hidapi)
else
	ifeq ($(UNAME), Linux)
		CFLAGS += $(shell pkg-config --cflags hidapi-libusb)
		LDFLAGS = $(shell pkg-config --libs hidapi-libusb)
	else
		LDFLAGS = -lhidapi
	endif
endif

all: scythe.c footswitch.c common.h common.c debug.h debug.c
	$(CC) footswitch.c common.c debug.c -o footswitch $(CFLAGS) $(LDFLAGS)
	$(CC) scythe.c common.c debug.c -o scythe $(CFLAGS) $(LDFLAGS)

install: all
	$(INSTALL) $(PROGNAME) /usr/local/bin
ifeq ($(UNAME), Linux)
	$(INSTALLDATA) 19-footswitch.rules /etc/udev/rules.d
endif

clean:
	rm -f scythe footswitch *.o

