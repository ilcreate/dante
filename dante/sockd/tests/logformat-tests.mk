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

logformat_logger_test.o: $(srcdir)/tests/logformat_logger_test.c $(srcdir)/../lib/log.c $(srcdir)/../include/logjson.h
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

logformat_iolog_test.o: $(srcdir)/tests/logformat_iolog_test.c $(srcdir)/tests/logformat_logger_test.c $(srcdir)/../lib/log.c $(srcdir)/../include/logjson.h
	$(COMPILE) -UNDEBUG -c $(srcdir)/tests/logformat_iolog_test.c -o $@

logformat_iolog_test: $(filter-out sockd.o log.o,$(sockd_OBJECTS)) logformat_server_test.o logformat_iolog_test.o
	$(LINK) $(filter-out sockd.o log.o,$(sockd_OBJECTS)) logformat_server_test.o logformat_iolog_test.o $(sockd_LDADD) $(LIBS)

.PHONY: check-logformat-iolog
check-logformat-iolog: logformat_iolog_test
	python3 $(srcdir)/tests/logformat_iolog_test.py ./logformat_iolog_test -v

.PHONY: check-logformat-iolog-integration
check-logformat-iolog-integration: sockd
	python3 $(srcdir)/tests/logformat_iolog_integration_test.py ./sockd -v

logformat_session_test.o: $(srcdir)/tests/logformat_session_test.c $(srcdir)/tests/logformat_logger_test.c $(srcdir)/sockd_io.c $(srcdir)/../lib/log.c $(srcdir)/../include/logjson.h
	$(COMPILE) -UNDEBUG -c $(srcdir)/tests/logformat_session_test.c -o $@

logformat_session_test: $(filter-out sockd.o log.o sockd_io.o,$(sockd_OBJECTS)) logformat_server_test.o logformat_session_test.o
	$(LINK) $(filter-out sockd.o log.o sockd_io.o,$(sockd_OBJECTS)) logformat_server_test.o logformat_session_test.o $(sockd_LDADD) $(LIBS)

.PHONY: check-logformat-session
check-logformat-session: logformat_session_test
	python3 $(srcdir)/tests/logformat_session_test.py ./logformat_session_test -v

.PHONY: check-logformat-session-integration
check-logformat-session-integration: sockd
	python3 $(srcdir)/tests/logformat_session_integration_test.py ./sockd -v

logformat_diagnostic_test.o: $(srcdir)/tests/logformat_diagnostic_test.c
	$(COMPILE) -UNDEBUG -c $(srcdir)/tests/logformat_diagnostic_test.c -o $@

logformat_diagnostic_test: $(filter-out sockd.o,$(sockd_OBJECTS)) logformat_server_test.o logformat_diagnostic_test.o
	$(LINK) $(filter-out sockd.o,$(sockd_OBJECTS)) logformat_server_test.o logformat_diagnostic_test.o $(sockd_LDADD) $(LIBS)

.PHONY: check-logformat-diagnostic
check-logformat-diagnostic: logformat_diagnostic_test
	python3 $(srcdir)/tests/logformat_diagnostic_test.py ./logformat_diagnostic_test -v

logformat_special_test.o: $(srcdir)/tests/logformat_special_test.c $(srcdir)/../lib/log.c $(srcdir)/../include/logjson.h
	$(COMPILE) -UNDEBUG -c $(srcdir)/tests/logformat_special_test.c -o $@

logformat_special_test: $(filter-out sockd.o log.o,$(sockd_OBJECTS)) logformat_server_test.o logformat_special_test.o
	$(LINK) $(filter-out sockd.o log.o,$(sockd_OBJECTS)) logformat_server_test.o logformat_special_test.o $(sockd_LDADD) $(LIBS)

.PHONY: check-logformat-special
check-logformat-special: logformat_special_test
	python3 $(srcdir)/tests/logformat_special_test.py ./logformat_special_test -v

.PHONY: check-logformat-special-integration
check-logformat-special-integration: sockd
	python3 $(srcdir)/tests/logformat_special_integration_test.py ./sockd -v

.PHONY: check-logformat-integration
check-logformat-integration: sockd
	python3 $(srcdir)/tests/logformat_runtime_integration_test.py ./sockd -v

logformat_benchmark.o: $(srcdir)/tests/logformat_benchmark.c $(srcdir)/../include/logjson.h
	$(COMPILE) -UNDEBUG -c $(srcdir)/tests/logformat_benchmark.c -o $@

logformat_benchmark: $(filter-out sockd.o,$(sockd_OBJECTS)) logformat_server_test.o logformat_benchmark.o
	$(LINK) $(filter-out sockd.o,$(sockd_OBJECTS)) logformat_server_test.o logformat_benchmark.o $(sockd_LDADD) $(LIBS)

.PHONY: benchmark-logformat
benchmark-logformat: logformat_benchmark
	python3 $(srcdir)/tests/logformat_benchmark.py ./logformat_benchmark
