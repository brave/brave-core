#!/usr/bin/env bash

if [[ $# -lt "4" ]]; then
  echo "usage: $0 <app> <pkg_scripts> <component_plist> <pkg_out>"
  exit 1
fi

APP="${1}"
PKG_SCRIPTS="${2}"
COMPONENT_PLIST="${3}"
PKG_OUT="${4}"

PKG_ROOT=pkg_root

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

# Delete old pkg root directory, if any
if [[ -d "$PKG_ROOT" ]]; then
  rm -rf "$PKG_ROOT"
fi

# Create the pkg root directory
mkdir "$PKG_ROOT"

# Copy the application into the pkg root directory
cp -a "$APP" "$PKG_ROOT"

# Create a template component property list
pkgbuild --analyze --root "$PKG_ROOT" "$COMPONENT_PLIST"

# Mark the bundle as non-relocatable
plutil -replace BundleIsRelocatable -bool NO "$COMPONENT_PLIST"

# Create the pkg
pkgbuild --identifier com.brave.Brave \
	 --install-location /Applications \
	 --root "$PKG_ROOT" \
	 --scripts "$PKG_SCRIPTS" \
	 --component-plist "$COMPONENT_PLIST" \
	 "$PKG_OUT"
