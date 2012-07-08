CC	= gcc
CFLAGS	= -Wall -O3
LDFLAGS = -Wall -O3

all: cc-termios cc-ftdi

cc-termios: cc-termios.o

cc-ftdi: cc-ftdi.o
	$(CC) $(LDFLAGS) -o cc-ftdi cc-ftdi.o -lftdi -lusb
