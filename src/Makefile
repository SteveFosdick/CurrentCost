all: cc-termios cc-ftdi xml2csv ascii-clean cc-now.cgi cc-history.cgi cc-picker.cgi cgi-test test-db-logger xml2pg xml2sqlite ts2unix maxlen

DAEMON_MODULES = logger.o file-logger.o db-logger-pg.o pg-common.o daemon.o cc-common.o

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

CGI_TEST_MODULES = cgi-main.o cgi-test.o cc-html.o

cgi-test: $(CGI_TEST_MODULES)
	$(CC) $(LDFLAGS) -o cgi-test $(CGI_TEST_MODULES)

CGI_NOW_MODULES = cgi-main.o cgi-now.o cc-html.o parsefile.o textfile.o mapfile.o

cc-now.cgi: $(CGI_NOW_MODULES)
	$(CC) $(LDFLAGS) -o cc-now.cgi $(CGI_NOW_MODULES)

CGI_NOW_PG_MODULES = cgi-main.o cgi-dbmain.o cgi-now-pg.o log-db-err.o cc-html.o

cc-now-pg.cgi: $(CGI_NOW_MODULES)
	$(CC) $(LDFLAGS) -o cc-now.cgi $(CGI_NOW_PG_MODULES) -lpq

CGI_HIST_MODULES = cgi-main.o cgi-history.o cc-rusage.o cc-html.o history.o parsefile.o textfile.o mapfile.o

cc-history.cgi: $(CGI_HIST_MODULES)
	$(CC) $(LDFLAGS) -o cc-history.cgi $(CGI_HIST_MODULES)

CGI_PICKER_MODULES = cgi-main.o cgi-picker.o cc-html.o

cc-picker.cgi: $(CGI_PICKER_MODULES)
	$(CC) $(LDFLAGS) -o cc-picker.cgi $(CGI_PICKER_MODULES)

TEST_LOGGER_MODULES = testlogger.o logger.o file-logger.o db-logger-pg.o cc-common.o

testlogger: $(TEST_LOGGER_MODULES)
	$(CC) $(LDFLAGS) -o testlogger $(TEST_LOGGER_MODULES)

TEST_DB_LOGGER_MODULES = test-db-logger.o db-logger-pg.o pg-common.o cc-common.o

test-db-logger: $(TEST_DB_LOGGER_MODULES)
	$(CC) $(LDFLAGS) -o test-db-logger $(TEST_DB_LOGGER_MODULES) -lpq -lpthread

XML2PG_MODULES = xml2pg.o db-logger-pg.o pg-common.o cc-common.o

xml2pg: $(XML2PG_MODULES)
	$(CC) $(LDFLAGS) -o xml2pg $(XML2PG_MODULES) -lpq -lpthread

XML2SQLITE_MODULES = xml2sqlite.o parsefile.o textfile.o mapfile.o cc-common.o

xml2sqlite: $(XML2SQLITE_MODULES)
	$(CC) $(LDFLAGS) -o xml2sqlite $(XML2SQLITE_MODULES) -lsqlite3

cc-common.o:  cc-defs.h cc-common.h
cc-html.o: cc-defs.h cgi-main.h cc-html.h
cc-ftdi.o:  cc-common.h daemon.h logger.h
cc-rusage.o: cc-rusage.h
cc-termios.o:  cc-common.h daemon.h logger.h
cgi-history.o:  cgi-main.h cc-html.h cc-rusage.h history.h
cgi-now.o:  cgi-main.h cc-html.h parsefile.h textfile.h
cgi-picker.o:  cgi-main.h cc-html.h
cgi-test.o:  cgi-main.h cc-html.h
daemon.o:  cc-common.h daemon.h
db-logger-pg.o:  cc-common.h db-logger.h logger.h
file-logger.o:  cc-common.h file-logger.h logger.h
history.o:  cgi-main.h cc-html.h history.h parsefile.h textfile.h
logger.o:  cc-defs.h cc-common.h db-logger.h file-logger.h logger.h
mapfile.o:  cc-common.h mapfile.h
parsefile.o:  cc-common.h parsefile.h textfile.h
pg-common.o: cc-common.h pg-common.h
test-db-logger.o:  cc-defs.h cc-common.h db-logger.h logger.h
testlogger.o:  cc-common.h logger.h
textfile.o:  textfile.h
xml2csv.o:  cc-defs.h cc-common.h parsefile.h textfile.h
xml2dat.o:  cc-common.h parsefile.h textfile.h
xml2pg.o:  cc-defs.h cc-common.h
xml2sqlite.o:  cc-common.h parsefile.h textfile.h
