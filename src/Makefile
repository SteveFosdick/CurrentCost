all: cc-termios cc-ftdi xml2csv ascii-clean cc-now.cgi cc-history.cgi cc-picker.cgi cgi-test test-db-logger xml2pg xml2sqlite ts2unix maxlen

DAEMON_MODULES = logger.o file-logger.o db-logger-pg.o daemon.o cc-common.o

CC_TERMIOS_MODULES = cc-termios.o $(DAEMON_MODULES)

cc-termios: $(CC_TERMIOS_MODULES)
	$(CC) $(LDFLAGS) -o cc-termios $(CC_TERMIOS_MODULES) -lpq -lpthread

CC_FTDI_MODULES = cc-ftdi.o $(DAEMON_MODULES)

cc-ftdi.o: cc-ftdi.c
	$(CC) $(CFLAGS) $(FTDI_INC) -c $^

cc-ftdi: $(CC_FTDI_MODULES)
	$(CC) $(CFLAGS) $(LDFLAGS) -o cc-ftdi $(CC_FTDI_MODULES) $(FTDI_LIB) -lpq -lpthread

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

TEST_LOGGER_MODULES = testlogger.o logger.o file-logger.o db-logger-pg.o cc-common.o

testlogger: $(TEST_LOGGER_MODULES)
	$(CC) $(LDFLAGS) -o testlogger $(TEST_LOGGER_MODULES)

TEST_DB_LOGGER_MODULES = test-db-logger.o db-logger-pg.o cc-common.o

test-db-logger: $(TEST_DB_LOGGER_MODULES)
	$(CC) $(LDFLAGS) -o test-db-logger $(TEST_DB_LOGGER_MODULES) -lpq -lpthread

XML2PG_MODULES = xml2pg.o db-logger-pg.o  cc-common.o

xml2pg: $(XML2PG_MODULES)
	$(CC) $(LDFLAGS) -o xml2pg $(XML2PG_MODULES) -lpq -lpthread

XML2SQLITE_MODULES = xml2sqlite.o parsefile.o textfile.o mapfile.o cc-common.o

xml2sqlite: $(XML2SQLITE_MODULES)
	$(CC) $(LDFLAGS) -o xml2sqlite $(XML2SQLITE_MODULES) -lsqlite3

cc-common.o cc-termios.o cc-ftdi.o parsefile.o history.o: cc-common.h
cc-termios.o cc-ftdi.o: daemon.h logger.h
parsefile.o history.o cgi-now.o xml2csv.o: parsefile.h
history.o: history.h
logger.o cc-termios.o cc_ftdi.o: logger.h
logger.o file-logger.o: file-logger.h
logger.o db-logger-pg.o: db-logger.h

