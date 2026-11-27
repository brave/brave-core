#!/bin/sh
# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

# Runs the test suites of an isolate archive. All arguments are passed to each
# test binary. See build/isolate/README.md in brave-core.

set -eu

isolate_dir="$(dirname "$0")"

if ! command -v node >/dev/null 2>&1; then
  echo "error: node is not found in PATH" >&2
  exit 1
fi

node "$isolate_dir/runner/check_environment.cjs"

# exec replaces this shell with node, so signals (e.g. a CI timeout) reach the
# runner directly and its exit code is returned as is.
exec node "$isolate_dir/runner/run.ts" "$isolate_dir/manifest.json" "$@"
