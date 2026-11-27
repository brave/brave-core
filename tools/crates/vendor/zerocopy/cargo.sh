#!/usr/bin/env bash
#
# Copyright 2024 The Fuchsia Authors
#
# Licensed under a BSD-style license <LICENSE-BSD>, Apache License, Version 2.0
# <LICENSE-APACHE or https://www.apache.org/licenses/LICENSE-2.0>, or the MIT
# license <LICENSE-MIT or https://opensource.org/licenses/MIT>, at your option.
# This file may not be copied, modified, or distributed except according to
# those terms.

set -eo pipefail

ZEROCOPY_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" >/dev/null 2>&1 && pwd)"
REPO_DIR="$(dirname "$ZEROCOPY_DIR")"

# Build `cargo-zerocopy` without any RUSTFLAGS or CARGO_TARGET_DIR set in the
# environment. Build it from the repository root so that Zerocopy's vendoring
# config does not apply to the unvendored tools workspace. Prefer `stable`
# when installed, and fall back to `pinned-stable` from `Cargo.toml` so slim
# CI images do not have to ship both an unpinned and a pinned stable toolchain.
BUILD_TOOLCHAIN="stable"
if command -v rustup >/dev/null 2>&1; then
  INSTALLED_TOOLCHAINS="$(rustup toolchain list)"
  if [[ "$INSTALLED_TOOLCHAINS" != *$'\n'stable-* && "$INSTALLED_TOOLCHAINS" != stable-* ]]; then
    PINNED_STABLE="$(sed -n 's/^[[:space:]]*pinned-stable[[:space:]]*=[[:space:]]*"\([^"]*\)"[[:space:]]*\(#.*\)\{0,1\}$/\1/p' "$ZEROCOPY_DIR/Cargo.toml")"
    PINNED_STABLE="${PINNED_STABLE%%$'\n'*}"
    if [ -n "$PINNED_STABLE" ]; then
      BUILD_TOOLCHAIN="$PINNED_STABLE"
    fi
  fi
fi
(
  cd "$REPO_DIR"
  env -u RUSTFLAGS -u CARGO_TARGET_DIR cargo "+$BUILD_TOOLCHAIN" build --manifest-path tools/cargo-zerocopy/Cargo.toml -p cargo-zerocopy -q
)

cd "$ZEROCOPY_DIR"
exec "$REPO_DIR/tools/target/debug/cargo-zerocopy" "$@"
