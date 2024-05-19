PREFIX		:= /usr/local
UDEVPREFIX	:= /etc/udev

TARGETS	:= \
	footswitch \
	scythe \
	scythe2

INCDIR		:= include
SRCDIR		:= src
OBJDIR		:= obj

COMMONSRC	:= \
	common.c \
	debug.c

INSTALL	:= /usr/bin/install -c
INSTALLDATA	:= /usr/bin/install -c -m 644
CFLAGS		:= -Wall -I$(INCDIR)
UNAME		:= $(shell uname)

ifeq ($(UNAME), Darwin)
	CFLAGS	+= -DOSX $(shell pkg-config --cflags hidapi)
	LDLIBS	:= $(shell pkg-config --libs hidapi)
else
	ifeq ($(UNAME), Linux)
		CFLAGS	+= $(shell pkg-config --cflags hidapi-libusb)
		LDLIBS	:= $(shell pkg-config --libs hidapi-libusb)
	else
		LDLIBS	:= -lhidapi
	endif
endif

all: $(OBJDIR) $(TARGETS)

$(OBJDIR):
	mkdir $@

$(OBJDIR)/%.o: $(SRCDIR)/%.c
	$(CC) $(CFLAGS) -c -o $@ $<

$(TARGETS): %: $(patsubst %.c, $(OBJDIR)/%.o, $(COMMONSRC)) $(OBJDIR)/%.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDLIBS)

install: all
	$(INSTALL) -d $(DESTDIR)$(PREFIX)/bin
	for target in $(TARGETS); do \
		$(INSTALL) "$$target" $(DESTDIR)$(PREFIX)/bin; \
	done
ifeq ($(UNAME), Linux)
	$(INSTALL) -d $(DESTDIR)$(UDEVPREFIX)/rules.d
	$(INSTALLDATA) 19-footswitch.rules $(DESTDIR)$(UDEVPREFIX)/rules.d
endif

uninstall:
	rm -f $(addprefix $(DESTDIR)$(PREFIX)/bin/, $(TARGETS))
ifeq ($(UNAME), Linux)
	rm -f $(DESTDIR)$(UDEVPREFIX)/rules.d/19-footswitch.rules
endif

clean:
	rm -rf $(TARGETS) $(OBJDIR)

