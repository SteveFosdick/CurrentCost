CFLAGS	= -Wall -O3 -DDEFAULT_DIR='"/data/Misc/CurrentCost"' -DBASE_URL='"http://meije/cgi-bin/"'
LDFLAGS = -Wall -O3
VPATH=../src

include ../src/Makefile

CC_FTDI_MODULES = ../src/cc-ftdi.c $(DAEMON_MODULES)

cc-ftdi: $(CC_FTDI_MODULES)
	$(CC) $(CFLAGS) -I /usr/include/libftdi1 $(LDFLAGS) -o cc-ftdi $(CC_FTDI_MODULES) -lftdi1 -lusb
