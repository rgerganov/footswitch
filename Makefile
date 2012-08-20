PROGNAME = footswitch
CFLAGS = -Wall
LDFLAGS = -lusb-1.0

all: $(PROGNAME)

$(PROGNAME): $(PROGNAME).c common.h common.c debug.h debug.c
	$(CC) $(PROGNAME).c common.c debug.c -o $(PROGNAME) $(CFLAGS) $(LDFLAGS)

clean:
	rm -f $(PROGNAME) *.o

