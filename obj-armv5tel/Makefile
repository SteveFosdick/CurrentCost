VPATH=../src
CC	= gcc
CFLAGS	= -Wall -O3 -march=armv5te -DDEFAULT_DIR='"/share/fozzy/Data/CurrentCost"' -DBASE_URL='"http://fosdick.slyip.net/cgi-bin/"'
LDFLAGS = -Wall -O3 -march=armv5te

include ../src/Makefile

CC_FTDI_MODULES = cc-ftdi.o $(DAEMON_MODULES)

cc-ftdi: $(CC_FTDI_MODULES)
	$(CC) $(LDFLAGS) -o cc-ftdi $(CC_FTDI_MODULES) -lftdi -lusb

