#!/usr/bin/env bash

# Script to re-vendor the WIT files that wasi-rs uses as defined by a
# particular tag in upstream repositories.
#
# This script is executed on CI to ensure that everything is up-to-date.
set -ex

# Space-separated list of wasi proposals that are vendored here along with the
# tag that they're all vendored at.
#
# This assumes that the repositories all have the pattern:
# https://github.com/WebAssembly/wasi-$repo
# and every repository has a tag `v$tag` here. That is currently done as part
# of the WASI release process.
tag=0.2.4
dst=wit/deps

vendor() {
  dst="$1"
  tag="$2"
  subdir="$3"
  repos="$4"

  rm -rf $dst
  mkdir -p $dst

  for repo in $repos; do
    mkdir $dst/$repo
    curl -L https://github.com/WebAssembly/wasi-$repo/archive/refs/tags/v$tag.tar.gz | \
      tar xzf - --strip-components=2 -C $dst/$repo wasi-$repo-$tag/$subdir
    rm -rf $dst/$repo/deps*
  done
}

vendor crates/wasip2/wit/deps 0.2.4 wit "cli clocks filesystem http io random sockets"
vendor crates/wasip3/wit/deps 0.3.0-rc-2025-08-15 wit-0.3.0-draft "cli clocks filesystem http random sockets"

# WASIp1 vendoring logic
wasip1_rev="0ba0c5e2"
curl -o crates/wasip1/typenames.witx -L \
  https://raw.githubusercontent.com/WebAssembly/WASI/$wasip1_rev/phases/snapshot/witx/typenames.witx
curl -o crates/wasip1/wasi_snapshot_preview1.witx -L \
  https://raw.githubusercontent.com/WebAssembly/WASI/$wasip1_rev/phases/snapshot/witx/wasi_snapshot_preview1.witx
