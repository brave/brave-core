#!/usr/bin/env bash
# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

# Creates an Alpine Linux (musl) sysroot for `is_musl=true` builds without
# root or containers, using apk.static in user mode.
#
# Usage: create_sysroot.sh [--arch x86_64|aarch64] [--branch edge|v3.23]
# Output: //brave/build/linux/musl/sysroots/alpine_<branch>_<arch>

set -euo pipefail

ARCH=x86_64
BRANCH=edge
while [[ $# -gt 0 ]]; do
  case "$1" in
    --arch) ARCH="$2"; shift 2 ;;
    --branch) BRANCH="$2"; shift 2 ;;
    *) echo "unknown arg $1" >&2; exit 1 ;;
  esac
done

SRC_DIR="$(cd "$(dirname "$0")/../../../.." && pwd)"
OUT="$SRC_DIR/brave/build/linux/musl/sysroots/alpine_${BRANCH}_${ARCH}"
MIRROR="https://dl-cdn.alpinelinux.org/alpine"
WORK="$(mktemp -d)"
trap 'rm -rf "$WORK"' EXIT

curl -sfL "$MIRROR/$BRANCH/main/x86_64/APKINDEX.tar.gz" -o "$WORK/index.tgz"
APK_TOOLS_VER="$(tar xzOf "$WORK/index.tgz" APKINDEX |
  awk '/^P:apk-tools-static$/{f=1} f&&/^V:/{sub("V:","");print;f=0}')"
curl -sfL "$MIRROR/$BRANCH/main/x86_64/apk-tools-static-$APK_TOOLS_VER.apk" \
  -o "$WORK/apk.apk"
# The .apk's signature segment makes tar warn; only the payload matters.
tar xzf "$WORK/apk.apk" -C "$WORK" sbin/apk.static 2>/dev/null || true
APK="$WORK/sbin/apk.static"
[[ -x "$APK" ]] || { echo "failed to fetch apk.static" >&2; exit 1; }

PACKAGES=(
  musl-dev linux-headers bsd-compat-headers
  # crtbegin.o / libgcc for the clang driver; libstdc++ for the Qt shim.
  gcc g++
  alsa-lib-dev at-spi2-core-dev curl-dev bluez-dev cairo-dev cups-dev dbus-dev
  eudev-dev expat-dev fontconfig-dev freetype-dev glib-dev gtk+3.0-dev
  gtk4.0-dev krb5-dev libcap-dev libdrm-dev libffi-dev libva-dev
  libx11-dev libxcb-dev libxcomposite-dev libxdamage-dev libxext-dev
  libxfixes-dev libxkbcommon-dev libxrandr-dev libxshmfence-dev libxtst-dev
  mesa-dev nspr-dev nss-dev pango-dev pciutils-dev pipewire-dev
  pulseaudio-dev qt5-qtbase-dev qt6-qtbase-dev wayland-dev zlib-dev
)

rm -rf "$OUT"
mkdir -p "$OUT"
"$APK" --root "$OUT" --arch "$ARCH" --usermode --initdb --no-cache \
  --no-scripts --allow-untrusted \
  -X "$MIRROR/$BRANCH/main" -X "$MIRROR/$BRANCH/community" \
  add "${PACKAGES[@]}"

# Make absolute symlinks relative so the sysroot is relocatable.
find "$OUT" -type l -lname '/*' -print0 | while IFS= read -r -d '' l; do
  target="$(readlink "$l")"
  ln -sfn "$(realpath -m --relative-to="$(dirname "$l")" "$OUT$target")" "$l"
done

# Clang uses the <arch>-linux-musl triple; Alpine's GCC uses
# <arch>-alpine-linux-musl. Alias libstdc++'s target headers (for the Qt shim).
for d in "$OUT"/usr/include/c++/*/; do
  ln -sfn "$ARCH-alpine-linux-musl" "$d/$ARCH-linux-musl"
done

# Some musl/bsd-compat headers #warn on non-standard includes that Chromium
# deps use (e.g. <sys/poll.h>, <sys/cdefs.h>); -Werror would reject them.
sed -i -e '/^#warning redirecting incorrect #include/d' \
  -e '/^#warning usage of non-standard #include/d' "$OUT"/usr/include/sys/*.h

# Linker scripts may reference absolute paths; lld resolves them inside
# --sysroot, so nothing to rewrite for musl. Drop bulky unneeded content.
rm -rf "$OUT"/usr/share/{doc,man,locale,icons,gtk-doc} "$OUT"/var/cache

echo "Sysroot created at $OUT"
