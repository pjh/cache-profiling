INCL = -I./

CC 	= gcc
#CFLAGS 	= $(INCL) -O0 -g -Wall -Wunused -D_GNU_SOURCE
CFLAGS  = -D__STDC_CONSTANT_MACROS $(INCL) -D_GNU_SOURCE -Wall -static -O0

default: cache-trash
ALL = cache-trash
all: $(ALL)

COMMON = kp_macros.h kp_recovery.h ptlcalls.h

######################################################
cache-trash: cache-trash.c kp_recovery.c ptlcalls.c $(COMMON)
	$(CC) -o $@ $^ $(CFLAGS) -lpthread

######################################################
clean:
	rm -f *.o $(ALL)

