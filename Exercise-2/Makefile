############################################
# Makefile for forkFFT
# Author: Raphael Wirnsberger <e12220836@student.tuwien.ac.at>
# Date: 10.11.2023
# Description: This Makefile builds forkFFT
############################################

CC = gcc
CFLAGS = -std=c99 -pedantic -Wall -D_DEFAULT_SOURCE -D_BSD_SOURCE -D_SVID_SOURCE -D_POSIX_C_SOURCE=200809L -g
MASTER = supervisor
SLAVE = generator

all: $(MASTER) $(SLAVE)

$(MASTER): $(MASTER).c
	$(CC) $(CFLAGS) $(MASTER).c -o $(MASTER) -lrt -lpthread

$(SLAVE): $(SLAVE).c
	$(CC) $(CFLAGS) $(SLAVE).c -o $(SLAVE) -lrt -lpthread

clean:
	rm -f $(MASTER) $(SLAVE)
