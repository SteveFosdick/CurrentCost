all: cc-termios cc-ftdi xml2csv ascii-clean cc-now.cgi cc-history.cgi cc-picker.cgi cgi-test

cc-termios: cc-termios.o

CC_FTDI_MODULES = cc-ftdi.o cc-common.o

cc-ftdi: $(CC_FTDI_MODULES)
	$(CC) $(LDFLAGS) -o cc-ftdi $(CC_FTDI_MODULES) -lftdi -lusb

XML2CSV_MODULES = xml2csv.o parsefile.o textfile.o mapfile.o cc-common.o

xml2csv: $(XML2CSV_MODULES)
	$(CC) $(LDFLAGS) -o xml2csv $(XML2CSV_MODULES)

CGI_TEST_MODULES = cgi-test.o cgi-common.o cc-common.o

cgi-test: $(CGI_TEST_MODULES)
	$(CC) $(LDFLAGS) -o cgi-test $(CGI_TEST_MODULES)

CGI_NOW_MODULES = cgi-now.o cgi-common.o parsefile.o textfile.o mapfile.o cc-common.o

cc-now.cgi: $(CGI_NOW_MODULES)
	$(CC) $(LDFLAGS) -o cc-now.cgi $(CGI_NOW_MODULES)

CGI_HIST_MODULES = cgi-history.o cgi-common.o history.o parsefile.o textfile.o mapfile.o cc-common.o

cc-history.cgi: $(CGI_HIST_MODULES)
	$(CC) $(LDFLAGS) -o cc-history.cgi $(CGI_HIST_MODULES)

CGI_PICKER_MODULES = cgi-picker.o cgi-common.o cc-common.o

cc-picker.cgi: $(CGI_PICKER_MODULES)
	$(CC) $(LDFLAGS) -o cc-picker.cgi $(CGI_PICKER_MODULES)

cc-common.o cc-termios.o cc-ftdi.o parsefile.o history.o: cc-common.h
parsefile.o history.o cgi-now.o xml2csv.o: parsefile.h
history.o: history.h
