#!/bin/bash

set -e

usage() {
  echo "./update-adblockrust.sh [git-ref]"
  echo "    git-ref: A git SHA or branch name"
  exit 1
}

[ "$#" -eq 1 ] || usage

missingCommand() {
    echo >&2 "Updating adblock-rust requires the command: \033[1m$1\033[0m"
    exit 1
}

# First Check to see if they have the neccessary software installed
command -v rustup >/dev/null 2>&1 || missingCommand "rustup"
command -v cargo >/dev/null 2>&1 || missingCommand "cargo"
command -v cbindgen >/dev/null 2>&1 || missingCommand "cbindgen"

# Ensure we have rust targets up to date
rustup target add x86_64-apple-ios
rustup target add aarch64-apple-ios

if [ -d "adblock-rust-ffi" ]; then
  rm -rf "adblock-rust-ffi"
fi

git clone https://github.com/brave/adblock-rust-ffi.git
pushd adblock-rust-ffi
git checkout "$1"
cargo lipo --release
cbindgen -o ../AdblockRust/adblock_rust_lib.h
popd

./create-xcframework.sh

pcregrep -M -o1 '\[\[package\]\]\nname = "adblock"\nversion = "(.+)"' "./adblock-rust-ffi/Cargo.lock" > VERSION
echo $1 >> VERSION