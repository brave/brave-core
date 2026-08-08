#!/usr/bin/env bash
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

set -eEo pipefail

if ! [[ "$GHSA_IDS" =~ ^GHSA-[0-9a-z]{4}-[0-9a-z]{4}-[0-9a-z]{4}(,[[:space:]]?GHSA-[0-9a-z]{4}-[0-9a-z]{4}-[0-9a-z]{4})*$ ]]; then
  echo "::error::GHSA_IDS contains invalid entries. Expected format: GHSA-xxxx-xxxx-xxxx (comma or comma-space separated)"
  exit 1
fi

IFS=',' read -ra IDS <<< "$GHSA_IDS"
for id in "${IDS[@]}"; do
  FLAGS+=(--id "${id// /}")
done

socket config set defaultOrg "brave"
socket fix . "${FLAGS[@]}"
