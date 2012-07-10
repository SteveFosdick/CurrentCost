CC	= gcc
CFLAGS	= -Wall -O3
LDFLAGS = -Wall -O3

all: cc-termios cc-ftdi xml2csv cc-now.cgi

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
