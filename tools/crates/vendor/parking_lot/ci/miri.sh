#!/usr/bin/env bash

set -ex

export CARGO_NET_RETRY=5
export CARGO_NET_TIMEOUT=10

rustup toolchain install nightly --component miri
rustup override set nightly
cargo miri setup

# We use different seeds for the different features to get a bit more test coverage.
SEED=0
for FEATURE in arc_lock serde deadlock_detection nightly; do
    export MIRIFLAGS="-Zmiri-strict-provenance -Zmiri-seed=$SEED"
    if [ "$FEATURE" = "deadlock_detection" ]; then
        # There's a background thread that never goes away, so Miri can't check for leaks.
        MIRIFLAGS="$MIRIFLAGS -Zmiri-ignore-leaks"
    fi
    cargo miri test --features=$FEATURE
    SEED=$(( $SEED+1 ))
done
