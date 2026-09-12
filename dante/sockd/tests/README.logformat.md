# Log format configuration tests

After configuring and building Dante, run from the source root:

```sh
make -C sockd -f Makefile -f tests/logformat-tests.mk check-logformat
```

This runs `sockd -V` checks, a daemon with SIGHUP reloads, and a C driver
using the real server initialization, parser, reset and configuration copy
routines. The tests need permission to create loopback and Unix sockets.
They use temporary configurations and stop the daemon when finished.

If the static client library is also built (the default), check that client
configuration rejects the server-only directive:

```sh
make -C sockd -f Makefile -f tests/logformat-tests.mk check-logformat-client
```

To run all checks together after building both drivers:

```sh
python3 sockd/tests/logformat_config_test.py sockd/sockd \
  --lifecycle-binary sockd/logformat_lifecycle_test \
  --client-binary sockd/logformat_client_test -v
```

These tests cover stage 1 of `docs/plans/logformat-json.md`. JSON log
serialization is implemented in later stages. The SIGHUP test observes the
mother's effective format; child inheritance and configuration copying are
checked separately by the C driver. It does not observe the format inside an
existing worker after a live reload.

The shipped parser was regenerated with GNU Bison 3.8.2 and the scanner with
Apple flex 2.6.4, using the rules in `lib/Makefile.am` and `LC_ALL=C` for the
legacy source encoding. Trailing whitespace in the generated scanner was
removed.
