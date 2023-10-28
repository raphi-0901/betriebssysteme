CC = gcc
CFLAGS = -std=c99 -pedantic -Wall -D_DEFAULT_SOURCE -D_BSD_SOURCE -D_SVID_SOURCE -D_POSIX_C_SOURCE=200809L -g -c
PNAME = forkfft

all: $(PNAME)

$(PNAME): main.c
	$(CC) $(CFLAGS) -o $(PNAME) main.c

clean:
	rm -f $(PNAME)
