FILES=main.c config.c arguments.c configdefaults.c ircmod.c

PROGNAME=irccmd
LIBDIR=/usr/lib/
INCLUDES=/usr/include
LIBS=-llua5.1 -largtable2 -lircclient

CC=gcc
WARN= -O2 -Wall -fPIC -W -Wall
INCS=-I$(INCLUDES)


CFLAGS=$(WARN) $(INCS) $(LIBS) -D PROGNAME=$(PROGNAME)

all: $(PROGNAME)

$(PROGNAME):
	$(CC) $(CFLAGS) -o $@ $(FILES)

clean:
	rm -f $(FILES:%.c=%.o)
	rm -f $(PROGNAME)

