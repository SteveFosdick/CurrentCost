CC	= gcc
CFLAGS	= -Wall -O3
LDFLAGS = -Wall -O3

all: cc-termios cc-ftdi xml2csv

cc-termios: cc-termios.o

CC_FTDI_MODULES = cc-ftdi.o cc-common.o

cc-ftdi: $(CC_FTDI_MODULES)
	$(CC) $(LDFLAGS) -o cc-ftdi $(CC_FTDI_MODULES) -lftdi -lusb

XML2CSV_MODULES = xml2csv.o parsefile.o cc-common.o

xml2csv: $(XML2CSV_MODULES)
	$(CC) $(LDFLAGS) -o xml2csv $(XML2CSV_MODULES)
