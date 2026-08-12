#!/usr/bin/env bash
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

set -eEo pipefail

[[ "$LEO_REF" =~ ^[0-9a-f]{40}$ ]] || { echo "::error::Invalid leo_ref: must be a 40-char hex SHA"; exit 1; }
sed -i -E "s|(\"@brave/leo\"[^#]*#)[0-9a-f]{40}|\1${LEO_REF}|" package.json
grep -q "$LEO_REF" package.json || { echo "::error::Failed to update Leo hash in package.json"; exit 1; }
