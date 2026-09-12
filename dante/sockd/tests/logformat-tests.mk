# Use alongside the configured sockd/Makefile:
# make -C sockd -f Makefile -f tests/logformat-tests.mk check-logformat

logformat_lifecycle_test.o: $(srcdir)/tests/logformat_lifecycle_test.c $(srcdir)/sockd.c
	$(COMPILE) -UNDEBUG -c $(srcdir)/tests/logformat_lifecycle_test.c -o $@

logformat_lifecycle_test: $(filter-out sockd.o,$(sockd_OBJECTS)) logformat_lifecycle_test.o
	$(LINK) $(filter-out sockd.o,$(sockd_OBJECTS)) logformat_lifecycle_test.o $(sockd_LDADD) $(LIBS)

.PHONY: check-logformat
check-logformat: sockd logformat_lifecycle_test
	python3 $(srcdir)/tests/logformat_config_test.py ./sockd --lifecycle-binary ./logformat_lifecycle_test -v

logformat_client_test: $(srcdir)/tests/logformat_client_test.c ../lib/.libs/libsocks.a
	$(CC) $(DEFS) $(DEFAULT_INCLUDES) $(INCLUDES) $(CPPFLAGS) $(CFLAGS) -DSOCKS_CLIENT=1 -DSOCKS_SERVER=0 -DSOCKSLIBRARY_DYNAMIC=0 -DSTANDALONE_UNIT_TEST=0 -o $@ $(srcdir)/tests/logformat_client_test.c ../lib/.libs/libsocks.a $(LIBS)

.PHONY: check-logformat-client
check-logformat-client: logformat_client_test
	python3 $(srcdir)/tests/logformat_config_test.py ./sockd --client-binary ./logformat_client_test -v LogformatConfigTest.test_client_config_rejects_logformat

logformat_server_test.o: $(srcdir)/sockd.c
	$(COMPILE) -Dmain=sockd_program_main -c $(srcdir)/sockd.c -o $@

logformat_logger_test.o: $(srcdir)/tests/logformat_logger_test.c $(srcdir)/../lib/log.c
	$(COMPILE) -UNDEBUG -c $(srcdir)/tests/logformat_logger_test.c -o $@

logformat_logger_test: $(filter-out sockd.o log.o,$(sockd_OBJECTS)) logformat_server_test.o logformat_logger_test.o
	$(LINK) $(filter-out sockd.o log.o,$(sockd_OBJECTS)) logformat_server_test.o logformat_logger_test.o $(sockd_LDADD) $(LIBS)

.PHONY: check-logformat-logger
check-logformat-logger: logformat_logger_test
	python3 $(srcdir)/tests/logformat_logger_test.py ./logformat_logger_test -v

logjson_test: $(srcdir)/tests/logjson_test.c $(srcdir)/../lib/logjson.c $(srcdir)/../include/logjson.h
	$(CC) $(CPPFLAGS) $(CFLAGS) -UNDEBUG -I$(srcdir)/../include -o $@ $(srcdir)/tests/logjson_test.c $(srcdir)/../lib/logjson.c

.PHONY: check-logjson
check-logjson: logjson_test
	python3 $(srcdir)/tests/logjson_test.py ./logjson_test -v
