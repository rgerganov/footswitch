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

all: footswitch scythe scythe2

footswitch: footswitch.c common.c debug.c
scythe: scythe.c common.c debug.c
scythe2: scythe2.c common.c debug.c

install: all
	$(INSTALL) -d $(DESTDIR)$(PREFIX)/bin
	$(INSTALL) footswitch $(DESTDIR)$(PREFIX)/bin
	$(INSTALL) scythe $(DESTDIR)$(PREFIX)/bin
ifeq ($(UNAME), Linux)
	$(INSTALL) -d $(DESTDIR)$(UDEVPREFIX)/rules.d
	$(INSTALLDATA) 19-footswitch.rules $(DESTDIR)$(UDEVPREFIX)/rules.d
endif

uninstall: 

	rm -f $(DESTDIR)$(PREFIX)/bin/footswitch
	rm -f $(DESTDIR)$(PREFIX)/bin/scythe
ifeq ($(UNAME), Linux)
	rm -f $(DESTDIR)$(UDEVPREFIX)/rules.d/19-footswitch.rules
endif


clean:
  rm -f scythe scythe2 footswitch *.o
  rm -f scythe footswitch *.o
