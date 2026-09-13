#!/bin/bash
# Build additional CI variants from the same exported release archive as packages.
set -euo pipefail

variant=${1:?usage: build-test-variant.sh client|livedebug INPUT_DIR OUTPUT_DIR}
case "$variant" in
  client|livedebug) ;;
  *) echo "unknown build variant: $variant" >&2; exit 2 ;;
esac
input_dir=$(cd "${2:?}" && pwd)
mkdir -p "${3:?}"
output_dir=$(cd "$3" && pwd)
build_dir=$(mktemp -d "${TMPDIR:-/tmp}/dante-$variant.XXXXXX")
trap 'rm -rf "$build_dir"' EXIT

source_name=$(python3 -c 'import json,sys; print(json.load(open(sys.argv[1]))["source"])' \
  "$input_dir/build-metadata.json")
version=$(python3 -c 'import json,sys; print(json.load(open(sys.argv[1]))["version"])' \
  "$input_dir/build-metadata.json")
tar -xzf "$input_dir/$source_name" -C "$build_dir"
source_dir="$build_dir/dante-$version"
cd "$source_dir"

configure_options=(
  --without-pam --without-gssapi --without-sasl --without-ldap
  --without-upnp --without-libwrap --without-bsdauth
)
case "$variant" in
  client) ;;
  livedebug) configure_options+=(--disable-client --enable-livedebug) ;;
esac

./configure "${configure_options[@]}" > "$output_dir/configure-$variant.log" 2>&1 || {
  tail -100 "$output_dir/configure-$variant.log"
  exit 1
}
cp config.log "$output_dir/config-$variant.log"
make -j"${BUILD_JOBS:-2}" > "$output_dir/build-$variant.log" 2>&1 || {
  tail -100 "$output_dir/build-$variant.log"
  exit 1
}

case "$variant" in
  client)
    make -C sockd -f Makefile -f tests/logformat-tests.mk \
      check-logformat-client > "$output_dir/test-$variant.log" 2>&1 || {
        tail -200 "$output_dir/test-$variant.log"
        exit 1
      }
    ;;
  livedebug)
    bash ci/run-server-tests.sh --livedebug \
      > "$output_dir/test-$variant.log" 2>&1 || {
        tail -200 "$output_dir/test-$variant.log"
        exit 1
      }
    ;;
esac
