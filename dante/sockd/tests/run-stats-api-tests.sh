#!/bin/sh

set -eu

project_root=$(CDPATH= cd -- "$(dirname -- "$0")/../.." && pwd)
test_binary=$(mktemp "${TMPDIR:-/tmp}/dante-stats-api-test.XXXXXX")
trap 'rm -f "$test_binary"' EXIT HUP INT TERM

test_cc=${CC:-cc}

"$test_cc" \
   -DHAVE_CONFIG_H=1 \
   -DSTANDALONE_UNIT_TEST=1 \
   -DSOCKD_STATS_TEST=1 \
   -DSOCKS_CLIENT=0 \
   -DSOCKS_SERVER=1 \
   -I"$project_root/include" \
   -I"$project_root/lib" \
   -I"$project_root/libscompat" \
   "$project_root/sockd/tests/stats_api_test.c" \
   "$project_root/sockd/statistics.c" \
   -o "$test_binary"

"$test_binary"
