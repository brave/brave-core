#!/bin/bash
# Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0 OR ISC

set -euo pipefail

# This script tests aws-lc-rs integration with the rustls ecosystem (rcgen, webpki, rustls).
# It uses Cargo's [patch.crates-io] feature to override dependencies, which is more robust
# than modifying individual dependency declarations.

function usage() {
  cat << EOF
Usage: $(basename "$0") [OPTIONS]

Tests aws-lc-rs integration with the rustls ecosystem.

Options:
  --latest-release  Test against latest stable releases (instead of main branch)
  --cleanup         Automatically delete cloned repositories on exit
  --help            Show this help message

Dependencies for --latest-release: jq, cargo-show, cargo-download
EOF
}

latest_release=0
auto_cleanup=0
for arg in "$@"; do
  case "$arg" in
    --help)
      usage
      exit 0
      ;;
    --latest-release)
      latest_release=1
      ;;
    --cleanup)
      auto_cleanup=1
      ;;
    # GitHub Actions emits the literal string "false" when
    # ${{ matrix.latest == 1 && '--latest-release' }} is unset.
    false | "")
      ;;
    *)
      echo "Unknown option: $arg" >&2
      usage >&2
      exit 1
      ;;
  esac
done

function require_cmd() {
  local cmd="$1"
  local hint="$2"
  if ! command -v "$cmd" >/dev/null 2>&1; then
    echo "Missing dependency: ${hint}" >&2
    exit 1
  fi
}

# cargo-show / cargo-download / jq are only used to resolve published crate
# versions. The default (main-branch) path does not need them.
function check_latest_release_dependencies() {
  [[ $latest_release == "1" ]] || return 0
  require_cmd jq "jq"
  require_cmd cargo-show "cargo-show (cargo install cargo-show)"
  require_cmd cargo-download "cargo-download (cargo install cargo-download)"
}
check_latest_release_dependencies

ROOT="${GITHUB_WORKSPACE:-$(git rev-parse --show-toplevel)}"

CLEANUP_ON_EXIT=()

function cleanup_cloned_repos() {
  if [ ${#CLEANUP_ON_EXIT[@]} -eq 0 ]; then
    return
  fi
  if [ "$auto_cleanup" -eq 0 ]; then
    echo "You can delete the following directories:"
    echo "${CLEANUP_ON_EXIT[@]}"
  else
    for x in "${CLEANUP_ON_EXIT[@]}"; do
      echo "Deleting: ${x}"
      rm -rf "${x}"
    done
  fi
}

trap cleanup_cloned_repos EXIT

# Get the latest stable (non-prerelease) version of a crate from crates.io
function get_latest_stable_version() {
  local crate="$1"
  local version
  version="$(cargo show --json "$crate" | jq -r '
    [.versions[] |
     select(.yanked == false and (.num | test("alpha|beta|rc") | not))
    ][0].num
  ')"
  if [[ -z "$version" || "$version" == "null" ]]; then
    echo "Failed to determine latest stable version of ${crate}" >&2
    exit 1
  fi
  echo "$version"
}

# Get the git commit SHA for a specific crate version from crates.io
function get_crate_commit() {
  local crate="$1"
  local version="$2"
  local tmp_dir sha
  tmp_dir="$(mktemp -d)"

  cargo download -o "$tmp_dir/crate.tar.gz" "${crate}=${version}"
  tar xzf "$tmp_dir/crate.tar.gz" -C "$tmp_dir" --strip-components=1
  sha="$(jq -r '.git.sha1' "$tmp_dir/.cargo_vcs_info.json")"
  rm -rf "$tmp_dir"
  if [[ -z "$sha" || "$sha" == "null" ]]; then
    echo "Failed to read git SHA for ${crate} ${version}" >&2
    exit 1
  fi
  echo "$sha"
}

# Add [patch.crates-io] entries to override aws-lc-rs and aws-lc-sys
# Usage: add_aws_lc_patch <cargo_toml_path> <aws_lc_rs_workspace_root>
function add_aws_lc_patch() {
  local cargo_toml="$1"
  local aws_lc_workspace="$2"

  if grep -q "aws-lc-rs = { path = \"${aws_lc_workspace}" "$cargo_toml"; then
    echo "Patch already present in $cargo_toml"
    return
  fi

  local aws_lc_rs_patch="aws-lc-rs = { path = \"${aws_lc_workspace}/aws-lc-rs\" }"
  local aws_lc_sys_patch="aws-lc-sys = { path = \"${aws_lc_workspace}/aws-lc-sys\" }"

  if grep -q '^\[patch\.crates-io\]' "$cargo_toml"; then
    local tmp_file
    tmp_file="$(mktemp)"
    awk -v rs="$aws_lc_rs_patch" -v sys="$aws_lc_sys_patch" '
      { print }
      $0 == "[patch.crates-io]" { print rs; print sys }
    ' "$cargo_toml" > "$tmp_file"
    mv "$tmp_file" "$cargo_toml"
  else
    cat >> "$cargo_toml" << EOF

[patch.crates-io]
${aws_lc_rs_patch}
${aws_lc_sys_patch}
EOF
  fi
}

# Shallow-clone a repository. When a commit is given, fetch only that object.
# Usage: clone_repo <url> <destination> [commit]
function clone_repo() {
  local url="$1"
  local dest="$2"
  local commit="${3:-}"

  if [ -n "$commit" ]; then
    git init "$dest"
    git -C "$dest" remote add origin "$url"
    git -C "$dest" fetch --depth 1 origin "$commit"
    git -C "$dest" checkout --detach FETCH_HEAD
    # Do not shallow-clone submodules here: the recorded SHA is often not a
    # branch tip, so --depth 1 cannot resolve it.
    git -C "$dest" submodule update --init --recursive
  else
    git clone --depth 1 --recurse-submodules --shallow-submodules "$url" "$dest"
  fi
}

# Clone, patch aws-lc-rs into the workspace, and refresh the lockfile.
# Leaves the caller in the cloned directory (pair with popd).
# Usage: setup_upstream <git_url> <crates_io_name>
function setup_upstream() {
  local url="$1"
  local crate="$2"
  local dest
  dest="$(mktemp -d)"
  CLEANUP_ON_EXIT+=("$dest")

  if [[ $latest_release == "1" ]]; then
    local version commit
    version="$(get_latest_stable_version "$crate")"
    commit="$(get_crate_commit "$crate" "$version")"
    echo "Using ${crate} version ${version} (commit: ${commit})"
    clone_repo "$url" "$dest" "$commit"
  else
    clone_repo "$url" "$dest"
  fi

  pushd "$dest" > /dev/null
  add_aws_lc_patch "Cargo.toml" "$ROOT"
  # Keep upstream's committed Cargo.lock: its pre-release deps (e.g. rustls-ring
  # and rustls 0.24.0-dev.N) only build together at the locked versions.
  if [[ -f Cargo.lock ]]; then
    cargo update -p aws-lc-rs -p aws-lc-sys
  else
    cargo generate-lockfile
  fi
}

echo "=== Testing rcgen with aws-lc-rs ==="

setup_upstream "https://github.com/rustls/rcgen" "rcgen"
cargo tree -i aws-lc-rs --features aws_lc_rs
cargo test --features aws_lc_rs
popd > /dev/null

echo "=== Testing rustls-webpki with aws-lc-rs ==="

setup_upstream "https://github.com/rustls/webpki.git" "rustls-webpki"
# Extract just the [features] section and check for aws-lc-rs feature there.
FEATURES_SECTION=$(sed -n '/^\[features\]/,/^\[/p' Cargo.toml)
if echo "$FEATURES_SECTION" | grep -qE '^aws(-|_)lc(-|_)rs\s*='; then
  WEBPKI_FEATURE="aws-lc-rs"
  cargo tree -i aws-lc-rs --features "$WEBPKI_FEATURE"
  cargo test --features "$WEBPKI_FEATURE"
else
  # No aws-lc-rs feature - newer structure uses rustls-aws-lc-rs dev-dependency
  echo "No aws-lc-rs feature found, running tests with default configuration"
  cargo tree -i aws-lc-rs
  cargo test
fi
popd > /dev/null

echo "=== Testing rustls with aws-lc-rs ==="

setup_upstream "https://github.com/rustls/rustls.git" "rustls"

# Detect which package exercises aws-lc-rs.
# <=0.23.x: aws-lc-rs feature is in rustls/Cargo.toml
# early 0.24.x: aws-lc-rs feature is in rustls-test/Cargo.toml
# current main: dedicated rustls-aws-lc-rs provider crate; rustls-test
# always runs both providers and has no aws-lc-rs feature.
if grep -q '^aws-lc-rs\s*=' ./rustls/Cargo.toml; then
  # Old structure: aws-lc-rs feature is in the main rustls crate
  pushd ./rustls
  cargo tree -i aws-lc-rs --features aws-lc-rs
  cargo test --features aws-lc-rs
  popd > /dev/null # ./rustls
elif [ -d ./rustls-aws-lc-rs ]; then
  # Latest structure: aws-lc-rs lives in the rustls-aws-lc-rs provider crate
  cargo tree -i aws-lc-rs -p rustls-aws-lc-rs
  cargo test -p rustls-aws-lc-rs
  cargo test -p rustls-test
elif grep -q '^aws-lc-rs\s*=' ./rustls-test/Cargo.toml; then
  # Intermediate structure: aws-lc-rs feature is in rustls-test
  pushd ./rustls-test
  cargo tree -i aws-lc-rs --features aws-lc-rs
  cargo test --features aws-lc-rs
  popd > /dev/null # ./rustls-test
else
  echo "Unable to locate aws-lc-rs usage in rustls workspace" >&2
  exit 1
fi
popd > /dev/null # rustls clone

echo "=== All rustls integration tests passed ==="
