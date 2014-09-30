CC=gcc
CFLAGS=-Wall
UNAME := $(shell uname -s)

ifeq ($(UNAME), Linux)
all: client_linux server_linux

client_linux: myftpclient.c
	$(CC) -o $@ $<
	
server_linux: myftpserver.c
	$(CC) -D Linux -o $@ $< -lpthread
	
clean:
	rm -rf client_linux server_linux
endif

ifeq ($(UNAME), SunOS)
all: client_unix server_unix

client_unix: myftpclient.c
	$(CC) -o $@ $< -lsocket -lnsl

server_unix: myftpserver.c
	$(CC) -D SunOS -o $@ $< -lsocket -lnsl -lpthread
	
clean:
	rm -rf client_unix server_unix
endif


ifeq ($(UNAME), Darwin)
all: client_unix server_unix

client_unix: myftpclient.c
	$(CC) -o $@ $< 

server_unix: myftpserver.c
	$(CC) -D Darwin -o $@ $< -lpthread
	
clean:
	rm -rf client_mac server_mac
endif
