#!/bin/bash
set -euo pipefail

OUT_DIR="bin"
LIB_NAME="libboringtun.dylib"

echo "Building for x86_64-apple-darwin..."
cargo build --release --target x86_64-apple-darwin

echo "Building for aarch64-apple-darwin..."
cargo build --release --target aarch64-apple-darwin

X86_LIB=$(find target/x86_64-apple-darwin -name "$LIB_NAME" | head -1)
ARM_LIB=$(find target/aarch64-apple-darwin -name "$LIB_NAME" | head -1)

echo "x86_64: $X86_LIB"
echo "aarch64: $ARM_LIB"

mkdir -p "$OUT_DIR"

echo "Creating Universal Binary with lipo..."
lipo -create "$X86_LIB" "$ARM_LIB" -output "${OUT_DIR}/${LIB_NAME}"

echo "Done: ${OUT_DIR}/${LIB_NAME}"
lipo -info "${OUT_DIR}/${LIB_NAME}"
