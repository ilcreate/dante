#!/bin/bash
# Build distro-compatible Debian packages from the committed source archive.
set -euo pipefail

input_dir=$(cd "${1:?usage: build-deb.sh INPUT_DIR OUTPUT_DIR TARGET}" && pwd)
mkdir -p "${2:?}"
output_dir=$(cd "$2" && pwd)
target=${3:?}
case "$target" in
  ubuntu22.04) distribution=jammy ;;
  debian12) distribution=bookworm ;;
  *) echo "unknown Debian target: $target" >&2; exit 2 ;;
esac

build_dir=$(mktemp -d "${TMPDIR:-/tmp}/dante-deb.XXXXXX")
source_dir=""
diagnostics="$output_dir/diagnostics"
mkdir -p "$diagnostics"
collect_diagnostics() {
  status=$?
  trap - EXIT
  if [[ -n "$source_dir" && -f "$source_dir/config.log" ]]; then
    cp "$source_dir/config.log" "$output_dir/config.log"
  fi
  shopt -s nullglob
  for artifact in "$build_dir"/*.deb "$build_dir"/*.ddeb \
      "$build_dir"/*.changes "$build_dir"/*.buildinfo; do
    cp "$artifact" "$diagnostics/"
  done
  rm -rf "$build_dir"
  exit "$status"
}
trap collect_diagnostics EXIT
metadata="$input_dir/build-metadata.json"
source_name=$(python3 -c 'import json,sys; print(json.load(open(sys.argv[1]))["source"])' "$metadata")
version=$(python3 -c 'import json,sys; print(json.load(open(sys.argv[1]))["version"])' "$metadata")
base_package_version=$(python3 -c 'import json,sys; print(json.load(open(sys.argv[1]))["package_version"])' "$metadata")
package_version="$base_package_version+$target"
tar -xzf "$input_dir/$source_name" -C "$build_dir"
source_dir="$build_dir/dante-$version"

cd "$source_dir"
export DEBFULLNAME="Dante CI"
export DEBEMAIL="ci@example.invalid"
export DEBIAN_FRONTEND=noninteractive
dch --newversion "$package_version" --distribution "$distribution" \
  --force-distribution "Automated build for $target."

# Resolve the authoritative Build-Depends from the packaged control file.
mk-build-deps --install --remove \
  --tool 'apt-get -y --no-install-recommends' debian/control \
  > "$output_dir/build-dependencies.log" 2>&1

dpkg_build_arch=$(dpkg --print-architecture)
case "$dpkg_build_arch" in
  amd64|arm64) ;;
  *) echo "unsupported Debian build architecture: $dpkg_build_arch" >&2; exit 1 ;;
esac

dpkg-buildpackage -b -us -uc 2>&1 | tee "$output_dir/build.log"
server_package="$build_dir/dante-server_${package_version}_${dpkg_build_arch}.deb"
if [[ ! -f "$server_package" ]]; then
  echo "expected server package missing: $server_package" >&2
  exit 1
fi
actual_version=$(dpkg-deb -f "$server_package" Version)
actual_arch=$(dpkg-deb -f "$server_package" Architecture)
actual_dependencies=$(dpkg-deb -f "$server_package" Depends)
dpkg-deb -f "$server_package" | tee "$output_dir/package-control.log"
dpkg-deb -c "$server_package" | tee "$output_dir/package-contents.log"
[[ "$actual_version" == "$package_version" ]] || {
  echo "package version mismatch: expected $package_version, got $actual_version" >&2
  exit 1
}
[[ "$actual_arch" == "$dpkg_build_arch" ]] || {
  echo "package architecture mismatch: expected $dpkg_build_arch, got $actual_arch" >&2
  exit 1
}
grep -Eq '[./]usr/sbin/danted$' "$output_dir/package-contents.log"
grep -Eq '[./]lib/systemd/system/danted.service$' "$output_dir/package-contents.log"
if grep -Eq '[./](usr/sbin/sockd|etc/sockd.conf|lib/systemd/system/sockd.service)$' \
    "$output_dir/package-contents.log"; then
  echo "server package contains legacy custom sockd paths" >&2
  exit 1
fi

control_dir="$build_dir/control"
dpkg-deb -e "$server_package" "$control_dir"
grep -Fxq '/etc/danted.conf' "$control_dir/conffiles" || {
  echo "/etc/danted.conf is not registered as a conffile" >&2
  exit 1
}

inspect_dir="$build_dir/inspect"
dpkg-deb -x "$server_package" "$inspect_dir"
"$inspect_dir/usr/sbin/danted" -vv 2>&1 | tee "$output_dir/danted-vv.log"
grep -Eq '^build:.*\bpam\b' "$output_dir/danted-vv.log" || {
  echo "danted was built without PAM" >&2; exit 1;
}
grep -Eq '^build:.*\blibwrap\b' "$output_dir/danted-vv.log" || {
  echo "danted was built without libwrap" >&2; exit 1;
}

changes=("$build_dir"/*.changes)
if [[ ! -e "${changes[0]}" ]]; then
  echo "dpkg-buildpackage did not produce a .changes file" >&2
  exit 1
fi
lintian --fail-on error "${changes[@]}" 2>&1 | tee "$output_dir/lintian.log"

cp "$server_package" "$output_dir/"

build_info="$output_dir/$(basename "$server_package").build-info.json"
python3 -c '
import json, platform, sys
source, destination, target, package_version, base_version, architecture, dependencies = sys.argv[1:]
data = json.load(open(source))
data.update({
    "format": "deb",
    "target": target,
    "architecture": architecture,
    "build_platform": platform.platform(),
    "base_package_version": base_version,
    "package_version": package_version,
    "dependencies": dependencies,
    "features": "server; PAM; libwrap; statistics API",
})
with open(destination, "w") as output:
    json.dump(data, output, indent=2)
    output.write("\n")
' "$metadata" "$build_info" "$target" "$package_version" "$base_package_version" "$dpkg_build_arch" "$actual_dependencies"
