#!/usr/bin/env bash

set -euo pipefail

if [[ $# -lt "4" ]]; then
  echo "usage: $0 <pkg_src> <pkg_dst> <mac_signing_keychain> <mac_installer_signing_identifier>"
  exit 1
fi

SOURCE="${1}"
DEST="${2}"
MAC_SIGNING_KEYCHAIN="${3}"
MAC_INSTALLER_SIGNING_IDENTIFIER="${4}"

function check_exit() {
    return=$?;
    if [[ $return -eq 0 ]]; then
	echo "[INFO] $0 succeeded"
    else
	echo "[ERROR] $0 failed"
    fi

    exit $return
}

trap check_exit EXIT

if [[ -f $DEST ]]; then
  rm -f "$DEST"
fi

/usr/bin/productsign --sign "$MAC_INSTALLER_SIGNING_IDENTIFIER" --keychain "$MAC_SIGNING_KEYCHAIN" "$SOURCE" "$DEST"
