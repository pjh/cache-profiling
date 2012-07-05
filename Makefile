INCL = -I./

CC 	= gcc
CFLAGS 	= $(INCL) -O0 -g -Wall -Wunused -D_GNU_SOURCE

default: cache-trash
ALL = cache-trash
all: $(ALL)

COMMON = kp_macros.h kp_recovery.h

######################################################
cache-trash: cache-trash.c kp_recovery.c $(COMMON)
	$(CC) -o $@ $^ $(CFLAGS)

######################################################
clean:
	rm -f *.o $(ALL)

