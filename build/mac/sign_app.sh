#!/usr/bin/env bash

set -euo pipefail

if [[ ${#} -lt "7" ]]; then
  echo "usage: $0 <input_dir> <output_dir> <packaging_dir> <is_development> <mac_provisioning_profile> <mac_signing_keychain> <mac_signing_identifier>"
  exit 1
fi

SOURCE_DIR="${1}"
DEST_DIR="${2}"
PKG_DIR="${3}"
DEVELOPMENT=
MAC_PROVISIONING_PROFILE=
if [[ "${4}" = "True" ]]; then
    DEVELOPMENT="--development"
else
    MAC_PROVISIONING_PROFILE="${5}"
fi
MAC_SIGNING_KEYCHAIN="${6}"
MAC_SIGNING_IDENTIFIER="${7}"

if [[ -z ${7} ]]; then
  echo "Error: mac_signing_identifier is empty. Cannot sign."
  exit 1
fi

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

# brave/scripts/signing_helper.py will retrieve this value when called from
# sign_chrome.py
export MAC_PROVISIONING_PROFILE

# Clear output directory. It seems GN auto-creates directory path to the
# expected outputs. However, the signing script doesn't expect the path to
# have been created and fails trying to create it again.
echo "Cleaning $DEST_DIR ..."
rm -rf $DEST_DIR/*

# Invoke python script to do the signing.
PARAMS="--input $SOURCE_DIR --output $DEST_DIR --keychain $MAC_SIGNING_KEYCHAIN --identity $MAC_SIGNING_IDENTIFIER --disable-packaging --no-notarize"
if [[ -z "${DEVELOPMENT}" ]]; then
  # Copy mac_provisioning_profile to the packaging_dir since that's where the
  # signing scripts expects to find it.
  cp -f "$MAC_PROVISIONING_PROFILE" "$PKG_DIR"
else
  PARAMS="$PARAMS $DEVELOPMENT"
fi
"${PKG_DIR}/sign_chrome.py" $PARAMS
