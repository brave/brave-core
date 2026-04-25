#!/usr/bin/env bash

# Helper script for publishing all the Walrus crates.
#
# Usage:
#
#     ./publish.sh

set -eux

cd "$(dirname "$0")/crates/macro"
cargo publish

# Let crates.io's index notice that we published the macro.
sleep 10

cd ../..
cargo publish
