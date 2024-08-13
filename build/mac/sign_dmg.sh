#!/usr/bin/env bash

set -euo pipefail

if [[ $# -lt "5" ]]; then
  echo "usage: $0 <dmg_path>"
  exit 1
fi

SOURCE="${1}"
DEST="${2}"
MAC_SIGNING_KEYCHAIN="${3}"
MAC_SIGNING_IDENTIFIER="${4}"
REQUIREMENTS="${5}"

app_name=$(basename "$SOURCE")

set -v

function check_exit() {
    return=$?;
    if [[ $return -eq 0 ]]; then
  echo "[INFO] $0 succeded"
    else
  echo "[ERROR] $0 failed"
    fi

    exit $return
}

trap check_exit EXIT

if [[ -f $DEST ]]; then
  rm -f "$DEST"
fi

cp "$SOURCE" "$DEST"

set -v

codesign --force --options runtime --timestamp --sign "$MAC_SIGNING_IDENTIFIER" --keychain "$MAC_SIGNING_KEYCHAIN" "$DEST" "$REQUIREMENTS"

codesign -vvvvd "$DEST"
codesign --verify --strict --deep -vvvv "$DEST"
