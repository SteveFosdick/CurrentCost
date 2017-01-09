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

CGI_NOW_MODULES = cgi-main.o cgi-dbmain.o cgi-now.o log-db-err.o cc-common.o

cc-now.cgi: $(CGI_NOW_MODULES)
	$(CC) $(LDFLAGS) -o cc-now.cgi $(CGI_NOW_MODULES) -lpq

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

cc-common.o:  cc-common.h
cc-ftdi.o:  cc-common.h daemon.h logger.h
cc-termios.o:  cc-common.h daemon.h logger.h
cgi-common.o:  cc-common.h cgi-common.h
cgi-history.o:  cc-common.h cgi-common.h history.h
cgi-now.o:  cc-common.h cgi-common.h parsefile.h textfile.h
cgi-picker.o:  cc-common.h cgi-common.h
cgi-test.o:  cgi-common.h
daemon.o:  cc-common.h daemon.h
db-logger-pg.o:  cc-common.h db-logger.h logger.h
file-logger.o:  cc-common.h file-logger.h logger.h
history.o:  cc-common.h history.h parsefile.h textfile.h
histtest.o:  cc-common.h history.h
logger.o:  cc-common.h db-logger.h file-logger.h logger.h
mapfile.o:  cc-common.h mapfile.h
parsefile.o:  cc-common.h parsefile.h textfile.h
test-db-logger.o:  cc-common.h db-logger.h logger.h
testlogger.o:  cc-common.h logger.h
textfile.o:  textfile.h
xml2csv.o:  cc-common.h parsefile.h textfile.h
xml2dat.o:  cc-common.h parsefile.h textfile.h
xml2pg.o:  cc-common.h
xml2sqlite.o:  cc-common.h parsefile.h textfile.h
