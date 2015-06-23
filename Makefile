INSTALL = /usr/bin/install -c
INSTALLDATA = /usr/bin/install -c -m 644
PROGNAME = footswitch
CFLAGS = -Wall
UNAME := $(shell uname)
ifeq ($(UNAME), Darwin)
	CFLAGS += -DOSX
	LDFLAGS = -lhidapi
else
	ifeq ($(UNAME), Linux)
		LDFLAGS = `pkg-config hidapi-libusb --libs`
	else
		LDFLAGS = -lhidapi
	endif
endif

all: $(PROGNAME)

$(PROGNAME): $(PROGNAME).c common.h common.c debug.h debug.c
	$(CC) $(PROGNAME).c common.c debug.c -o $(PROGNAME) $(CFLAGS) $(LDFLAGS)

install: all
	$(INSTALL) $(PROGNAME) /usr/bin
ifeq ($(UNAME), Linux)
	$(INSTALLDATA) 19-footswitch.rules /etc/udev/rules.d
endif

clean:
	rm -f $(PROGNAME) *.o

