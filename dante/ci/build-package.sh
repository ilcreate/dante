#!/bin/bash
# Build in a disposable directory, always from the release source archive.
set -euo pipefail
format=${1:?usage: build-package.sh deb|rpm|macos INPUT_DIR OUTPUT_DIR}
input_dir=$(cd "${2:?}" && pwd)
mkdir -p "${3:?}"
output_dir=$(cd "$3" && pwd)
ci_dir=$(cd "$(dirname "$0")" && pwd)
build_dir=$(mktemp -d "${TMPDIR:-/tmp}/dante-package.XXXXXX")
trap 'rm -rf "$build_dir"' EXIT
source_name=$(python3 -c 'import json,sys; print(json.load(open(sys.argv[1]))["source"])' "$input_dir/build-metadata.json")
tar -xzf "$input_dir/$source_name" -C "$build_dir"
version=$(python3 -c 'import json,sys; print(json.load(open(sys.argv[1]))["version"])' "$input_dir/build-metadata.json")
source_dir="$build_dir/dante-$version"
cd "$source_dir"

case "$format" in
  deb|rpm) prefix=/usr; conf=/etc/sockd.conf; pid=/run/sockd/sockd.pid ;;
  macos)
    prefix=/usr/local; conf=/usr/local/etc/sockd.conf; pid=/var/run/sockd.pid
    # Do not accidentally link the distributable to Homebrew libraries.
    export PATH=/usr/bin:/bin:/usr/sbin:/sbin
    export CC=/usr/bin/clang
    export MACOSX_DEPLOYMENT_TARGET=15.0
    ;;
  *) exit 2 ;;
esac
export CFLAGS="-O2 -g -fstack-protector-strong"
export CPPFLAGS="-D_FORTIFY_SOURCE=2"
if [[ "$format" != macos ]]; then
  export LDFLAGS="-Wl,-z,relro,-z,now"
fi

./configure --disable-client --prefix="$prefix" \
  --with-sockd-conf="$conf" --with-pidfile="$pid" \
  --without-pam --without-gssapi --without-sasl --without-ldap \
  --without-upnp --without-libwrap --without-bsdauth > "$output_dir/configure.log" 2>&1 || {
    tail -100 "$output_dir/configure.log"; exit 1;
  }
cp config.log "$output_dir/config.log"
make -j"${BUILD_JOBS:-2}" > "$output_dir/build.log" 2>&1 || {
  tail -100 "$output_dir/build.log"; exit 1;
}
bash ci/run-server-tests.sh
./sockd/sockd -v
make DESTDIR="$build_dir/install" install > "$output_dir/install.log" 2>&1
# Packaging uses an explicit payload: no client libraries from server-only install.
python3 "$ci_dir/package.py" "$format" "$source_dir" "$input_dir/build-metadata.json" "$output_dir"
