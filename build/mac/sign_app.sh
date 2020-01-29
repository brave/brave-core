#!/usr/bin/env bash

set -euo pipefail

usage() {
  echo "usage: $0 <input_dir> <output_dir> <packaging_dir> <is_development> <mac_provisioning_profile> <mac_signing_identifier> <notarize> <notary_user> <notary_password> <notary_asc_provider>"
}

if [[ ${#} -lt "7" ]]; then
  usage
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
MAC_SIGNING_IDENTIFIER="${6}"

if [[ -z ${6} ]]; then
  echo "Error: mac_signing_identifier is empty. Cannot sign."
  exit 1
fi

if [[ ${#} -gt "6" ]]; then
  if [[ "${7}" = "True" ]]; then
    NOTARIZE="--notarize"
    NOTARY_USER="${8}"
    NOTARY_PASSWORD="${9}"
    if [[ -n "${NOTARIZE}" ]]; then
        if [[ ( -z "${NOTARY_USER}" ) || ( -z "${NOTARY_PASSWORD}" ) ]]; then
            echo "Error: when <notarize> is True, both <notary_user> and <notary_password> must be provided. Cannot perform notarization."
            usage
            exit 1
        fi
    fi
    NOTARIZE_ARGS="${NOTARIZE} --notary-user $NOTARY_USER --notary-password $NOTARY_PASSWORD"
    if [[ "${10}" != "" ]]; then
      NOTARY_ASC_PROVIDER="${10}"
      NOTARIZE_ARGS="$NOTARIZE_ARGS --notary-asc-provider ${NOTARY_ASC_PROVIDER}"
    fi
  else
    NOTARIZE="False"
  fi
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
if [ -d $DEST_DIR ]; then
  echo "Cleaning $DEST_DIR ..."
  rm -rf $DEST_DIR/*
fi

# Invoke python script to do the signing.
PARAMS="--input $SOURCE_DIR --output $DEST_DIR --identity $MAC_SIGNING_IDENTIFIER --disable-packaging"
if [[ -z "${DEVELOPMENT}" ]]; then
  # Copy mac_provisioning_profile to the packaging_dir since that's where the
  # signing scripts expects to find it.
  cp -f "$MAC_PROVISIONING_PROFILE" "$PKG_DIR"
else
  PARAMS="$PARAMS $DEVELOPMENT"
fi

if [[ "${NOTARIZE}" = "False" ]]; then
  PARAMS="$PARAMS --no-notarize"
else
  PARAMS="$PARAMS ${NOTARIZE_ARGS}"
fi

"${PKG_DIR}/sign_chrome.py" $PARAMS
