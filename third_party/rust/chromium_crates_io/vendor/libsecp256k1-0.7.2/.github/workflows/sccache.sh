#!/bin/bash

set -ex

export SCCACHE_CACHE_SIZE="1G"
export SCCACHE_IDLE_TIMEOUT=0
export SCCACHE_DIR="$HOME/sccache"
OS=$1
VERSION="0.2.13"

echo "Current OS: $OS"
case $OS in
"macOS")
  PLATFORM="x86_64-apple-darwin"
  ;;
"Linux")
  PLATFORM="x86_64-unknown-linux-musl"
  ;;
"Windows")
  PLATFORM="x86_64-pc-windows-msvc"
  VERSION="0.2.14"
  ;;
esac
echo "Target arch: " $PLATFORM
BASENAME="sccache-$VERSION-$PLATFORM"
URL="https://github.com/mozilla/sccache/releases/download/$VERSION/$BASENAME.tar.gz"
echo "Download sccache from " "$URL"
curl -LO "$URL"
tar -xzvf "$BASENAME.tar.gz"
ls $BASENAME/
echo "$(pwd)/$BASENAME" >> "$GITHUB_PATH"
echo "RUSTC_WRAPPER=sccache" >> "$GITHUB_ENV"
./$BASENAME/sccache --start-server
