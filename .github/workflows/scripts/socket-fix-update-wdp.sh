#!/usr/bin/env bash
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

set -eEo pipefail

[[ "$WDP_REF" =~ ^[0-9a-f]{40}$ ]] || { echo "::error::Invalid wdp_ref: must be a 40-char hex SHA"; exit 1; }
sed -i -E "s|(\"vendor/web-discovery-project\"[^@]*@)[0-9a-f]{40}|\1${WDP_REF}|" DEPS
grep -q "$WDP_REF" DEPS || { echo "::error::Failed to update WDP hash in DEPS"; exit 1; }
