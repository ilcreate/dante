#!/bin/bash
set -euo pipefail

case ${1:-} in
  "") livedebug=false ;;
  --livedebug) livedebug=true ;;
  *) echo "usage: $0 [--livedebug]" >&2; exit 2 ;;
esac
if (( $# > 1 )); then
  echo "usage: $0 [--livedebug]" >&2
  exit 2
fi

cd "$(dirname "$0")/.."

required_sources=(
  configure
  include/logjson.h
  lib/config_parse.c
  lib/config_scan.c
  lib/logjson.c
  sockd/tests/logformat-tests.mk
  sockd/tests/logformat_runtime_integration_test.py
)
for source in "${required_sources[@]}"; do
  if [[ ! -f "$source" ]]; then
    echo "release source is missing $source" >&2
    exit 1
  fi
done

make -C sockd -f Makefile -f tests/logformat-tests.mk \
  check-logformat \
  check-logformat-logger \
  check-logjson \
  check-logformat-iolog \
  check-logformat-iolog-integration \
  check-logformat-session \
  check-logformat-session-integration \
  check-logformat-special \
  check-logformat-diagnostic \
  check-logformat-special-integration \
  check-logformat-integration

bash sockd/tests/run-stats-api-tests.sh
python3 ci/smoke.py ./sockd/sockd --relay

if [[ "$livedebug" == true ]]; then
  python3 sockd/tests/logformat_special_integration_test.py \
    ./sockd/sockd --livedebug -v
fi
