############################################
# Makefile for forkFFT
# Author: Raphael Wirnsberger <e12220836@student.tuwien.ac.at>
# Date: 10.11.2023
# Description: This Makefile builds the server for http
############################################

CC = gcc
CFLAGS = -std=c99 -pedantic -Wall -D_DEFAULT_SOURCE -D_BSD_SOURCE -D_SVID_SOURCE -D_POSIX_C_SOURCE=200809L -g
SERVER = server

all: $(SERVER)

$(SERVER): $(SERVER).c
	$(CC) $(CFLAGS) $(SERVER).c -o $(SERVER)

clean:
	rm -f $(SERVER)
