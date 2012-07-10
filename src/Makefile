CC	= gcc
CFLAGS	= -Wall -O3
LDFLAGS = -Wall -O3

all: cc-termios cc-ftdi xml2csv cc-now.cgi ascii-clean

cc-termios: cc-termios.o

CC_FTDI_MODULES = cc-ftdi.o cc-common.o

cc-ftdi: $(CC_FTDI_MODULES)
	$(CC) $(LDFLAGS) -o cc-ftdi $(CC_FTDI_MODULES) -lftdi -lusb

XML2CSV_MODULES = xml2csv.o parsefile.o cc-common.o

xml2csv: $(XML2CSV_MODULES)
	$(CC) $(LDFLAGS) -o xml2csv $(XML2CSV_MODULES)

CGI_NOW_MODULES = cgi-now.o parsefile.o cc-common.o

cc-now.cgi: $(CGI_NOW_MODULES)
	$(CC) $(LDFLAGS) -o cc-now.cgi $(CGI_NOW_MODULES)

cc-common.o cc-termios.o cc-ftdi.o parsefile.o: cc-common.h
parsefile.o cgi-now.o xml2csv.o: parsefile.h
