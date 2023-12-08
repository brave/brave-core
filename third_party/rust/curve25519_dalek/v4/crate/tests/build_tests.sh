#!/bin/bash

function match_and_report() {
    PATTERN=$1
    FILE=$2

    if grep -q "$PATTERN" "$FILE"; then
      echo build OK "$FILE" : "$PATTERN"
    else
      echo build ERROR "$FILE" : "$PATTERN"
      echo ">>>>>>>>>>>>>>>>>>>>>>>>>>"
      cat "$FILE"
      echo "<<<<<<<<<<<<<<<<<<<<<<<<<<"
      exit 1
    fi
}

# Assuming naively 64 bit host
cargo clean
OUT=build_1.txt
env RUSTFLAGS="--cfg curve25519_dalek_diagnostics=\"build\"" cargo build > "$OUT" 2>&1
match_and_report "curve25519_dalek_backend is 'simd'" "$OUT"
match_and_report "curve25519_dalek_bits is '64'" "$OUT"

# Override to 32 bits assuming naively 64 bit build host
cargo clean
OUT=build_2.txt
env RUSTFLAGS="--cfg curve25519_dalek_diagnostics=\"build\" --cfg curve25519_dalek_bits=\"32\"" cargo build > "$OUT" 2>&1
match_and_report "curve25519_dalek_backend is 'serial'" "$OUT"
match_and_report "curve25519_dalek_bits is '32'" "$OUT"

# Override to 64 bits on 32 bit target
cargo clean
OUT=build_3.txt
env RUSTFLAGS="--cfg curve25519_dalek_diagnostics=\"build\" --cfg curve25519_dalek_bits=\"64\"" cargo build --target i686-unknown-linux-gnu > "$OUT" 2>&1
match_and_report "curve25519_dalek_backend is 'serial'" "$OUT"
match_and_report "curve25519_dalek_bits is '64'" "$OUT"

# 32 bit target default
cargo clean
OUT=build_4.txt
env RUSTFLAGS="--cfg curve25519_dalek_diagnostics=\"build\"" cargo build --target i686-unknown-linux-gnu > "$OUT" 2>&1
match_and_report "curve25519_dalek_backend is 'serial'" "$OUT"
match_and_report "curve25519_dalek_bits is '32'" "$OUT"

# wasm 32 bit target default
cargo clean
OUT=build_5.txt
env RUSTFLAGS="--cfg curve25519_dalek_diagnostics=\"build\"" cargo build --target wasm32-unknown-unknown > "$OUT" 2>&1
match_and_report "curve25519_dalek_backend is 'serial'" "$OUT"
match_and_report "curve25519_dalek_bits is '32'" "$OUT"

# wasm 32 bit target default
# Attempted override w/ "simd" should result "serial" addition
cargo clean
OUT=build_5_1.txt
env RUSTFLAGS="--cfg curve25519_dalek_diagnostics=\"build\" --cfg curve25519_dalek_backend=\"simd\"" cargo build --target wasm32-unknown-unknown > "$OUT" 2>&1
# This overide must fail the compilation since "simd" is not available
# See: issues/532
match_and_report "Could not override curve25519_dalek_backend to simd" "$OUT"

# fiat override with default 64 bit naive host assumption
cargo clean
OUT=build_6.txt
env RUSTFLAGS="--cfg curve25519_dalek_diagnostics=\"build\" --cfg curve25519_dalek_backend=\"fiat\"" cargo build > "$OUT" 2>&1
match_and_report "curve25519_dalek_backend is 'fiat'" "$OUT"
match_and_report "curve25519_dalek_bits is '64'" "$OUT"

# fiat 32 bit override
cargo clean
OUT=build_7.txt
env RUSTFLAGS="--cfg curve25519_dalek_diagnostics=\"build\" --cfg curve25519_dalek_backend=\"fiat\" --cfg curve25519_dalek_bits=\"32\"" cargo build > "$OUT" 2>&1
match_and_report "curve25519_dalek_backend is 'fiat'" "$OUT"
match_and_report "curve25519_dalek_bits is '32'" "$OUT"

# serial override with default 64 bit naive host assumption
cargo clean
OUT=build_8.txt
env RUSTFLAGS="--cfg curve25519_dalek_diagnostics=\"build\" --cfg curve25519_dalek_backend=\"serial\"" cargo build > "$OUT" 2>&1
match_and_report "curve25519_dalek_backend is 'serial'" "$OUT"
match_and_report "curve25519_dalek_bits is '64'" "$OUT"

# serial 32 bit override
cargo clean
OUT=build_9.txt
env RUSTFLAGS="--cfg curve25519_dalek_diagnostics=\"build\" --cfg curve25519_dalek_backend=\"serial\" --cfg curve25519_dalek_bits=\"32\"" cargo build > "$OUT" 2>&1
match_and_report "curve25519_dalek_backend is 'serial'" "$OUT"
match_and_report "curve25519_dalek_bits is '32'" "$OUT"
