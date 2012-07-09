CC	= gcc
CFLAGS	= -Wall -O3
LDFLAGS = -Wall -O3

all: cc-termios cc-ftdi

cc-termios: cc-termios.o

CC_FTDI_MODULES = cc-ftdi.o cc-common.o

cc-ftdi: $(CC_FTDI_MODULES)
	$(CC) $(LDFLAGS) -o cc-ftdi $(CC_FTDI_MODULES) -lftdi -lusb
