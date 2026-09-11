#!/bin/bash
set -euo pipefail

case "${1:?usage: install-build-deps.sh deb|rpm}" in
  deb)
    apt-get update
    DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends \
      build-essential flex bison dpkg-dev ca-certificates python3 file
    ;;
  rpm)
    dnf install -y gcc make flex bison rpm-build python3 file tar gzip diffutils findutils
    ;;
  *) exit 2 ;;
esac
